
/*
 * EEZ Modular Firmware
 * Copyright (C) 2021-present, Envox d.o.o.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#if defined(__EMSCRIPTEN__)

#include <emscripten.h>

#include <stdlib.h>
#include <stdio.h>

#include <eez/flow/flow.h>
#include <eez/flow/expression.h>
#include <eez/flow/dashboard_api.h>

using namespace eez;
using namespace eez::gui;
using namespace eez::flow;

namespace eez {
namespace flow {

int getFlowStateIndex(FlowState *flowState) {
    return (int)((uint8_t *)flowState - ALLOC_BUFFER);
}

struct DashboardComponentExecutionState : public ComponenentExecutionState {
    ~DashboardComponentExecutionState() {
		EM_ASM({
            freeComponentExecutionState($0);
        }, state);
    }
	int32_t state;
};

} // flow
} // eez

static inline FlowState *getFlowState(int flowStateIndex) {
    return (FlowState *)(ALLOC_BUFFER + flowStateIndex);
}

static void updateArrayValue(ArrayValue *arrayValue1, ArrayValue *arrayValue2) {
    for (uint32_t i = 0; i < arrayValue1->arraySize; i++) {
        if (arrayValue1->values[i].getType() == VALUE_TYPE_ARRAY || arrayValue1->values[i].getType() == VALUE_TYPE_ARRAY_REF) {
            updateArrayValue(arrayValue1->values[i].getArray(), arrayValue2->values[i].getArray());
        } else {
            arrayValue1->values[i] = arrayValue2->values[i];
        }
    }
}

EM_PORT_API(Value *) createUndefinedValue() {
    auto pValue = ObjectAllocator<Value>::allocate(0x2e821285);
    *pValue = Value(0, VALUE_TYPE_UNDEFINED);
    return pValue;
}

EM_PORT_API(Value *) createNullValue() {
    auto pValue = ObjectAllocator<Value>::allocate(0x69debded);
    *pValue = Value(0, VALUE_TYPE_NULL);
    return pValue;
}

EM_PORT_API(Value *) createIntValue(int value) {
    auto pValue = ObjectAllocator<Value>::allocate(0x20ea356c);
    *pValue = Value(value, VALUE_TYPE_INT32);
    return pValue;
}

EM_PORT_API(Value *) createDoubleValue(double value) {
    auto pValue = ObjectAllocator<Value>::allocate(0xecfb69a9);
    *pValue = Value(value, VALUE_TYPE_DOUBLE);
    return pValue;
}

EM_PORT_API(Value *) createBooleanValue(int value) {
    auto pValue = ObjectAllocator<Value>::allocate(0x76071378);
    *pValue = Value(value, VALUE_TYPE_BOOLEAN);
    return pValue;
}

EM_PORT_API(Value *) createStringValue(const char *value) {
    auto pValue = ObjectAllocator<Value>::allocate(0x0a8a7ed1);
    Value stringValue = Value::makeStringRef(value, strlen(value), 0x5b1e51d7);
    *pValue = stringValue;
    return pValue;
}

EM_PORT_API(Value *) createArrayValue(int arraySize, int arrayType) {
    Value value = Value::makeArrayRef(arraySize, arrayType, 0xeabb7edc);
    auto pValue = ObjectAllocator<Value>::allocate(0xbab14c6a);
    if (pValue) {
        *pValue = value;
    }
    return pValue;
}

EM_PORT_API(Value *) createStreamValue(double value) {
    auto pValue = ObjectAllocator<Value>::allocate(0x53a2e660);
    *pValue = Value(value, VALUE_TYPE_STREAM);
    return pValue;
}

EM_PORT_API(Value *) createDateValue(int value) {
    auto pValue = ObjectAllocator<Value>::allocate(0x90b7ce70);
    *pValue = Value(value, VALUE_TYPE_DATE);
    return pValue;
}

EM_PORT_API(void) arrayValueSetElementValue(Value *arrayValuePtr, int elementIndex, Value *valuePtr) {
    auto array = arrayValuePtr->getArray();
    array->values[elementIndex] = *valuePtr;
}

EM_PORT_API(void) valueFree(Value *valuePtr) {
    ObjectAllocator<Value>::deallocate(valuePtr);
}

EM_PORT_API(void) setGlobalVariable(int globalVariableIndex, Value *valuePtr) {
    auto flowDefinition = static_cast<FlowDefinition *>(g_mainAssets->flowDefinition);
    Value *globalVariableValuePtr = flowDefinition->globalVariables[globalVariableIndex];
    *globalVariableValuePtr = *valuePtr;
}

EM_PORT_API(void) updateGlobalVariable(int globalVariableIndex, Value *valuePtr) {
    auto flowDefinition = static_cast<FlowDefinition *>(g_mainAssets->flowDefinition);
    Value *globalVariableValuePtr = flowDefinition->globalVariables[globalVariableIndex];
    updateArrayValue(globalVariableValuePtr->getArray(), valuePtr->getArray());
}

EM_PORT_API(int) getFlowIndex(int flowStateIndex) {
    auto flowState = getFlowState(flowStateIndex);
    return flowState->flowIndex;
}

EM_PORT_API(int) getComponentExecutionState(int flowStateIndex, int componentIndex) {
    auto flowState = getFlowState(flowStateIndex);
    auto component = flowState->flow->components[componentIndex];
    auto executionState = (DashboardComponentExecutionState *)flowState->componenentExecutionStates[componentIndex];
    if (executionState) {
        return executionState->state;
    }
    return -1;
}

EM_PORT_API(void) setComponentExecutionState(int flowStateIndex, int componentIndex, int state) {
    auto flowState = getFlowState(flowStateIndex);
    auto component = flowState->flow->components[componentIndex];
    auto executionState = (DashboardComponentExecutionState *)flowState->componenentExecutionStates[componentIndex];
    if (executionState) {
        if (state != -1) {
            executionState->state = state;
        } else {
            deallocateComponentExecutionState(flowState, componentIndex);
        }
    } else {
        if (state != -1) {
            executionState = allocateComponentExecutionState<DashboardComponentExecutionState>(flowState, componentIndex);
            executionState->state = state;
        }
    }
}

EM_PORT_API(const char *) getStringParam(int flowStateIndex, int componentIndex, int offset) {
    auto flowState = getFlowState(flowStateIndex);
    auto component = flowState->flow->components[componentIndex];
    auto ptr = (const uint32_t *)((const uint8_t *)component + sizeof(Component) + offset);
    return (const char *)(MEMORY_BEGIN + 4 + *ptr);
}

struct ExpressionList {
    uint32_t count;
    Value values[1];
};

EM_PORT_API(void *) getExpressionListParam(int flowStateIndex, int componentIndex, int offset) {
    auto flowState = getFlowState(flowStateIndex);
    auto component = flowState->flow->components[componentIndex];

    struct List {
        uint32_t count;
        uint32_t items;
    };
    auto list = (const List *)((const uint8_t *)component + sizeof(Component) + offset);

    auto expressionList = (ExpressionList *)::malloc((list->count + 1) * sizeof(Value));

    expressionList->count = list->count;

    auto items = (const uint32_t *)(MEMORY_BEGIN + 4 + list->items);

    for (uint32_t i = 0; i < list->count; i++) {
        // call Value constructor
        new (expressionList->values + i) Value();

        auto valueExpression = (const uint8_t *)(MEMORY_BEGIN + 4 + items[i]);
        if (!evalExpression(flowState, componentIndex, valueExpression, expressionList->values[i])) {
            throwError(flowState, componentIndex, "Failed to evaluate expression");
            return nullptr;
        }
    }

    return expressionList;
}

EM_PORT_API(void) freeExpressionListParam(void *ptr) {
    auto expressionList = (ExpressionList*)ptr;

    for (uint32_t i = 0; i < expressionList->count; i++) {
        // call Value desctructor
        (expressionList->values + i)->~Value();
    }

    ::free(ptr);
}

EM_PORT_API(Value *) evalProperty(int flowStateIndex, int componentIndex, int propertyIndex, int32_t *iterators) {
    auto flowState = getFlowState(g_mainAssets, flowStateIndex);

    Value result;
    if (!eez::flow::evalProperty(flowState, componentIndex, propertyIndex, result, nullptr, iterators)) {
        throwError(flowState, componentIndex, "Failed to evaluate property\n");
        return nullptr;
    }

    auto pValue = ObjectAllocator<Value>::allocate(0xb7e697b8);
    if (!pValue) {
        throwError(flowState, componentIndex, "Out of memory\n");
        return nullptr;
    }

    *pValue = result;

    return pValue;
}

EM_PORT_API(void) assignProperty(int flowStateIndex, int componentIndex, int propertyIndex, int32_t *iterators, Value *srcValuePtr) {
    auto flowState = getFlowState(g_mainAssets, flowStateIndex);

    Value dstValue;
    if (evalAssignableProperty(flowState, componentIndex, propertyIndex, dstValue, nullptr, iterators)) {
        assignValue(flowState, componentIndex, dstValue, *srcValuePtr);
    }
}

EM_PORT_API(void) setPropertyField(int flowStateIndex, int componentIndex, int propertyIndex, int fieldIndex, Value *valuePtr) {
    auto flowState = getFlowState(g_mainAssets, flowStateIndex);

    Value result;
    if (!eez::flow::evalProperty(flowState, componentIndex, propertyIndex, result)) {
        throwError(flowState, componentIndex, "Failed to evaluate property\n");
        return;
    }

	if (result.getType() == VALUE_TYPE_VALUE_PTR) {
		result = *result.pValueValue;
	}

    if (result.getType() != VALUE_TYPE_ARRAY && result.getType() != VALUE_TYPE_ARRAY_REF) {
        throwError(flowState, componentIndex, "Property is not an array");
        return;
    }

    auto array = result.getArray();

    if (fieldIndex < 0 || fieldIndex >= array->arraySize) {
        throwError(flowState, componentIndex, "Invalid field index");
        return;
    }

    array->values[fieldIndex] = *valuePtr;
    onValueChanged(array->values + fieldIndex);
}

EM_PORT_API(void) propagateValue(int flowStateIndex, int componentIndex, int outputIndex, Value *valuePtr) {
    auto flowState = getFlowState(g_mainAssets, flowStateIndex);
    eez::flow::propagateValue(flowState, componentIndex, outputIndex, *valuePtr);
}

EM_PORT_API(void) propagateValueThroughSeqout(int flowStateIndex, int componentIndex) {
    auto flowState = getFlowState(flowStateIndex);
	eez::flow::propagateValueThroughSeqout(flowState, componentIndex);
}

EM_PORT_API(void) startAsyncExecution(int flowStateIndex, int componentIndex) {
    auto flowState = getFlowState(flowStateIndex);
    eez::flow::startAsyncExecution(flowState, componentIndex);
}

EM_PORT_API(void) endAsyncExecution(int flowStateIndex, int componentIndex) {
    auto flowState = getFlowState(flowStateIndex);
    eez::flow::endAsyncExecution(flowState, componentIndex);
}

EM_PORT_API(void) executeCallAction(int flowStateIndex, int componentIndex, int flowIndex) {
    auto flowState = getFlowState(flowStateIndex);
    eez::flow::executeCallAction(flowState, componentIndex, flowIndex);
}

EM_PORT_API(void) throwError(int flowStateIndex, int componentIndex, const char *errorMessage) {
    auto flowState = getFlowState(flowStateIndex);
	eez::flow::throwError(flowState, componentIndex, errorMessage);
}

#endif // __EMSCRIPTEN__
