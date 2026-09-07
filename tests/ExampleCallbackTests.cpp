#include <AppAPI/ComSupport.hpp>
#include "Example.h"
#include <gtest/gtest.h>


/** State owned by the caller, reached through the "context" pointer. */
struct CalcState {
    int  value = 0;
    bool destroyed = false;
};

static HRESULT CalcGetValue (void* context, /*out*/int * value) {
    *value = static_cast<CalcState*>(context)->value;
    return S_OK;
}

static HRESULT CalcAdd (void* context, int a, int b, /*out*/int * result) {
    *result = a + b + static_cast<CalcState*>(context)->value;
    return S_OK;
}

static void CalcDestroy (void* context) {
    static_cast<CalcState*>(context)->destroyed = true;
}


TEST(ExampleCallbackTests, TestMethodsForwardToCallbacks) {
    CalcState state;
    state.value = 42;

    ICalcExtCallbacks callbacks{};
    callbacks.GetValue = CalcGetValue;
    callbacks.Add      = CalcAdd;
    callbacks.Destroy  = CalcDestroy;

    ICalcExt* obj = nullptr;
    HRESULT hr = ICalcExt_CreateCallback(&callbacks, &state, &obj);
    EXPECT_EQ(hr, S_OK);
    ASSERT_TRUE(obj);

    int val = 0;
    hr = obj->GetValue(&val); // inherited from ICalc
    EXPECT_EQ(hr, S_OK);
    EXPECT_EQ(val, 42);

    hr = obj->Add(1, 2, &val);
    EXPECT_EQ(hr, S_OK);
    EXPECT_EQ(val, 45);

    EXPECT_FALSE(state.destroyed);
    obj->Release();
    EXPECT_TRUE(state.destroyed);
}

TEST(ExampleCallbackTests, TestOmittedMethodIsNotImplemented) {
    CalcState state;

    ICalcExtCallbacks callbacks{};
    callbacks.GetValue = CalcGetValue; // Add deliberately left out

    ICalcExt* obj = nullptr;
    HRESULT hr = ICalcExt_CreateCallback(&callbacks, &state, &obj);
    EXPECT_EQ(hr, S_OK);

    int val = 0;
    hr = obj->Add(1, 2, &val);
    EXPECT_EQ(hr, E_NOTIMPL);

    obj->Release();
}

TEST(ExampleCallbackTests, TestQueryInterfaceCoversTheBaseInterface) {
    CalcState state;
    state.value = 42;

    ICalcExtCallbacks callbacks{};
    callbacks.GetValue = CalcGetValue;

    ICalcExt* obj = nullptr;
    HRESULT hr = ICalcExt_CreateCallback(&callbacks, &state, &obj);
    EXPECT_EQ(hr, S_OK);

    CComPtr<ICalc> base;
    hr = obj->QueryInterface(__uuidof(ICalc), (void**)&base);
    EXPECT_EQ(hr, S_OK);
    ASSERT_TRUE(base);

    int val = 0;
    hr = base->GetValue(&val);
    EXPECT_EQ(hr, S_OK);
    EXPECT_EQ(val, 42);

    obj->Release();
}

TEST(ExampleCallbackTests, TestNullArgumentsRejected) {
    CalcState state;
    ICalcExtCallbacks callbacks{};
    ICalcExt* obj = nullptr;

    EXPECT_EQ(ICalcExt_CreateCallback(nullptr, &state, &obj), E_POINTER);
    EXPECT_EQ(ICalcExt_CreateCallback(&callbacks, &state, nullptr), E_POINTER);
}
