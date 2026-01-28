#include <core/foundation/object.hpp>
#include <core/log/logger.hpp>
#include <core/error/assert.hpp>

// Ensure asserts are enabled for this test
#undef AP_ENABLE_ASSERTS
#define AP_ENABLE_ASSERTS 1

using namespace april::core;

class TestObject : public Object
{
public:
    TestObject(int id) : m_id(id) {
        AP_INFO("TestObject constructed: {}", m_id);
    }

    ~TestObject() override {
        AP_INFO("TestObject destroyed: {}", m_id);
    }

    auto getId() const -> int { return m_id; }

    // For testing breakable references
    ref<TestObject> m_child;
    BreakableReference<TestObject> m_parent{nullptr};

private:
    int m_id;
};

void testBasicRef()
{
    AP_INFO("--- testBasicRef ---");
    {
        ref<TestObject> pObj = make_ref<TestObject>(1);
        AP_ASSERT(pObj, "pObj should be valid");
        AP_ASSERT(pObj->getId() == 1, "Id match failed");
        AP_ASSERT(pObj->refCount() == 1, "RefCount should be 1");

        {
            ref<TestObject> pObj2 = pObj;
            AP_ASSERT(pObj->refCount() == 2, "RefCount should be 2");
            AP_ASSERT(pObj2->getId() == 1, "Id match failed");
        }
        AP_ASSERT(pObj->refCount() == 1, "RefCount should be 1");
    }
    AP_INFO("testBasicRef passed");
}

void testMoveSemantics()
{
    AP_INFO("--- testMoveSemantics ---");
    ref<TestObject> pObj = make_ref<TestObject>(2);
    AP_ASSERT(pObj->refCount() == 1, "RefCount should be 1");

    ref<TestObject> pObj2 = std::move(pObj);
    AP_ASSERT(!pObj, "pObj should be null after move");
    AP_ASSERT(pObj2, "pObj2 should be valid");
    AP_ASSERT(pObj2->refCount() == 1, "RefCount should be 1");
    AP_ASSERT(pObj2->getId() == 2, "Id match failed");

    AP_INFO("testMoveSemantics passed");
}

void testReset()
{
    AP_INFO("--- testReset ---");
    ref<TestObject> pObj = make_ref<TestObject>(3);
    AP_ASSERT(pObj->refCount() == 1, "RefCount should be 1");

    pObj.reset();
    AP_ASSERT(!pObj, "pObj should be null after reset");

    pObj = make_ref<TestObject>(4);
    AP_ASSERT(pObj->getId() == 4, "Id match failed");

    pObj = nullptr;
    AP_ASSERT(!pObj, "pObj should be null after assignment to nullptr");

    AP_INFO("testReset passed");
}

void testBreakableReference()
{
    AP_INFO("--- testBreakableReference ---");

    ref<TestObject> pParent = make_ref<TestObject>(10);
    ref<TestObject> pChild = make_ref<TestObject>(11);

    pParent->m_child = pChild;

    // Create cycle breaker
    pChild->m_parent = BreakableReference<TestObject>(pParent);

    AP_ASSERT(pChild->m_parent.get() == pParent.get(), "Parent pointer mismatch");

    // Break strong reference to avoid cycle if it were a strong ref
    pChild->m_parent.breakStrongReference();

    // Parent should still be alive because pParent holds it
    AP_ASSERT(pParent->refCount() == 1, "RefCount should be 1");
    AP_ASSERT(pChild->m_parent.get() == pParent.get(), "Parent pointer mismatch");

    pParent = nullptr;

    AP_INFO("testBreakableReference passed");
}

int main()
{
    try {
        testBasicRef();
        testMoveSemantics();
        testReset();
        testBreakableReference();

        AP_INFO("All tests passed!");
    } catch (std::exception const& e) {
        AP_ERROR("Test failed with exception: {}", e.what());
        return 1;
    }
    return 0;
}
