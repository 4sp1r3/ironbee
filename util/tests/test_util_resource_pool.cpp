//////////////////////////////////////////////////////////////////////////////
// Licensed to Qualys, Inc. (QUALYS) under one or more
// contributor license agreements.  See the NOTICE file distributed with
// this work for additional information regarding copyright ownership.
// QUALYS licenses this file to You under the Apache License, Version 2.0
// (the "License"); you may not use this file except in compliance with
// the License.  You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
/// @file
/// @brief IronBee --- Array Test Functions
///
/// @author Brian Rectanus <brectanus@qualys.com>
//////////////////////////////////////////////////////////////////////////////

#include "ironbee_config_auto.h"

#include <ironbee/mpool.h>
#include <ironbee/resource_pool.h>

#include "gtest/gtest.h"

namespace {
extern "C" {
    //! The resource we are going to build and test the resource pool with.
    struct resource_t {
        int preuse;
        int postuse;
        int use;
        int destroy;
    };
    typedef struct resource_t resource_t;

    //! Callback data for resource tests.
    struct cbdata_t {
        ib_mpool_t *mp;
    };
    typedef struct cbdata_t cbdata_t;

    ib_status_t create_fn(void **resource, void *data) {
        cbdata_t *cbdata = reinterpret_cast<cbdata_t *>(data);
        resource_t *tmp_r = reinterpret_cast<resource_t *>(
            ib_mpool_calloc(cbdata->mp, sizeof(*tmp_r), 1));
        *resource = tmp_r;
        return IB_OK;
    }

    ib_status_t destroy_fn(void *resource, void *data) {
        resource_t *r = reinterpret_cast<resource_t *>(resource);
        ++(r->destroy);
        return IB_OK;
    }
    ib_status_t preuse_fn(void *resource, void *data) {
        resource_t *r = reinterpret_cast<resource_t *>(resource);
        ++(r->preuse);
        return IB_OK;
    }
    ib_status_t postuse_fn(void *resource, void *data) {
        resource_t *r = reinterpret_cast<resource_t *>(resource);
        ++(r->postuse);
        return IB_OK;
    }

    struct use_fn_t {
        ib_status_t  rc;      /**< What does use_fn set rc to. */
        ib_status_t  user_rc; /**< What is returned by use_fn. */
        resource_t *resource; /**< Resource we get. */
    };
    typedef struct use_fn_t use_fn_t;

    ib_status_t use_fn(
        ib_resource_t *resource,
        ib_status_t   *user_rc,
        void          *cbdata
    )
    {
        use_fn_t *use = reinterpret_cast<use_fn_t *>(cbdata);
        resource_t *r = reinterpret_cast<resource_t *>(resource->resource);

        *user_rc = use->user_rc;

        ++(r->use);
        use->resource = r;

        return use->rc;
    }
} /* Close extern "C" */

} /* Close nonymous namespace. */

class ResourcePoolTest : public ::testing::Test {
public:
    virtual void SetUp()
    {
        ASSERT_EQ(IB_OK, ib_mpool_create(&m_mp, "ResourcePoolTest", NULL));
        m_cbdata.mp = m_mp;
        void *cbdata = reinterpret_cast<void *>(&m_cbdata);
        ASSERT_EQ(IB_OK, ib_resource_pool_create(
            &m_rp,
            m_mp,
            1,
            10,
            5,
            &create_fn,
            cbdata,
            &destroy_fn,
            cbdata,
            &preuse_fn,
            cbdata,
            &postuse_fn,
            cbdata
        ));
    }

    virtual void TearDown()
    {
        ib_mpool_release(m_mp);
    }
protected:
    cbdata_t m_cbdata;
    ib_mpool_t *m_mp;
    ib_resource_pool_t *m_rp;
};

TEST_F(ResourcePoolTest, create) {
    ASSERT_TRUE(m_mp);
    ASSERT_TRUE(m_rp);
}

TEST_F(ResourcePoolTest, get_return) {
    ib_resource_t *ib_r;
    resource_t *r;

    ASSERT_EQ(IB_OK, ib_resource_get(m_rp, true, &ib_r));

    r = reinterpret_cast<resource_t *>(ib_r->resource);
    ASSERT_TRUE(r);
    ASSERT_EQ(1U, ib_r->use);
    ASSERT_EQ(1, r->preuse);
    ASSERT_EQ(0, r->use);
    ASSERT_EQ(0, r->postuse);
    ASSERT_EQ(0, r->destroy);
    ++(r->use);

    ASSERT_EQ(IB_OK, ib_resource_return(ib_r));
    ASSERT_EQ(1, r->preuse);
    ASSERT_EQ(1, r->use);
    ASSERT_EQ(1, r->postuse);
    ASSERT_EQ(0, r->destroy);

    ASSERT_EQ(IB_OK, ib_resource_get(m_rp, true, &ib_r));
    ASSERT_EQ(2U, ib_r->use);
    ASSERT_EQ(IB_OK, ib_resource_return(ib_r));
    ASSERT_EQ(2, r->preuse);
    ASSERT_EQ(2, r->postuse);
    ASSERT_EQ(0, r->destroy);

    ASSERT_EQ(IB_OK, ib_resource_get(m_rp, true, &ib_r));
    ASSERT_EQ(3U, ib_r->use);
    ASSERT_EQ(IB_OK, ib_resource_return(ib_r));
    ASSERT_EQ(3, r->preuse);
    ASSERT_EQ(3, r->postuse);
    ASSERT_EQ(0, r->destroy);

    ASSERT_EQ(IB_OK, ib_resource_get(m_rp, true, &ib_r));
    ASSERT_EQ(4U, ib_r->use);
    ASSERT_EQ(IB_OK, ib_resource_return(ib_r));
    ASSERT_EQ(4, r->preuse);
    ASSERT_EQ(4, r->postuse);
    ASSERT_EQ(0, r->destroy);

    ASSERT_EQ(IB_OK, ib_resource_get(m_rp, true, &ib_r));
    ASSERT_EQ(5U, ib_r->use);
    ASSERT_EQ(IB_OK, ib_resource_return(ib_r));
    ASSERT_EQ(0U, ib_r->use);
    ASSERT_EQ(5, r->preuse);
    ASSERT_EQ(5, r->postuse);
    ASSERT_EQ(1, r->destroy);

    /* Get an return a resource. This should be no and NOT change r. */
    ASSERT_EQ(IB_OK, ib_resource_get(m_rp, true, &ib_r));
    ASSERT_EQ(IB_OK, ib_resource_return(ib_r));
    ASSERT_EQ(1U, ib_r->use);
    ASSERT_EQ(5, r->preuse);
    ASSERT_EQ(5, r->postuse);
    ASSERT_EQ(1, r->destroy);

    /* Get the new r and check it. */
    r = reinterpret_cast<resource_t *>(ib_r->resource);
    ASSERT_EQ(1, r->preuse);
    ASSERT_EQ(0, r->use);
    ASSERT_EQ(1, r->postuse);
    ASSERT_EQ(0, r->destroy);
}

TEST_F(ResourcePoolTest, use_ok) {
    /* Set IB_OK and return IB_OK. */
    use_fn_t use = { IB_OK, IB_OK, NULL };
    void *cbdata = reinterpret_cast<void *>(&use);
    ib_status_t rc;

    ASSERT_EQ(IB_OK, ib_resource_use(m_rp, false, use_fn, &rc, cbdata));
    ASSERT_EQ(IB_OK, rc);

    ASSERT_EQ(1, use.resource->use);
}

TEST_F(ResourcePoolTest, use_einval) {
    /* Set IB_OK and return IB_OK. */
    use_fn_t use;
    use.rc = IB_OK;
    use.user_rc = IB_EINVAL;
    use.resource = NULL;
    void *cbdata = reinterpret_cast<void *>(&use);
    ib_status_t rc;
    ib_resource_t *ib_r;
    resource_t *r;

    ASSERT_EQ(IB_OK, ib_resource_use(m_rp, false, use_fn, &rc, cbdata));
    ASSERT_EQ(IB_EINVAL, rc);

    ASSERT_EQ(1, use.resource->use);
    /* Get the same resource again. */
    ASSERT_EQ(IB_OK, ib_resource_get(m_rp, true, &ib_r));
    r = reinterpret_cast<resource_t *>(ib_r->resource);
    ASSERT_EQ(r, use.resource);
    ASSERT_EQ(IB_OK, ib_resource_return(ib_r));
}

TEST_F(ResourcePoolTest, use_corrupted_resource) {
    /* Set IB_OK and return IB_OK. */
    use_fn_t use;
    use.rc = IB_EINVAL;
    use.user_rc = IB_EOTHER;
    use.resource = NULL;
    void *cbdata = reinterpret_cast<void *>(&use);
    ib_status_t rc;
    ib_resource_t *ib_r;
    resource_t *r;

    ASSERT_EQ(IB_EINVAL, ib_resource_use(m_rp, false, use_fn, &rc, cbdata));
    ASSERT_EQ(IB_EOTHER, rc);

    ASSERT_EQ(1, use.resource->use);

    /* Get a new resource. */
    ASSERT_EQ(IB_OK, ib_resource_get(m_rp, true, &ib_r));
    r = reinterpret_cast<resource_t *>(ib_r->resource);
    ASSERT_NE(r, use.resource);
    ASSERT_EQ(IB_OK, ib_resource_return(ib_r));
}

TEST_F(ResourcePoolTest, limit_reached) {
    ib_resource_t *ib_r[11];

    for (int i = 0; i < 10; ++i) {
        ASSERT_EQ(IB_OK, ib_resource_get(m_rp, false, &ib_r[i]));
    }

    /* Fail to get the 11th resource. */
    ASSERT_EQ(IB_DECLINED, ib_resource_get(m_rp, false, &ib_r[10]));

    /* Return one, and get one. */
    ASSERT_EQ(IB_OK, ib_resource_return(ib_r[0]));
    ASSERT_EQ(IB_OK, ib_resource_get(m_rp, false, &ib_r[0]));

    /* Return them all. */
    for (int i = 0; i < 10; ++i) {
        ASSERT_EQ(IB_OK, ib_resource_return(ib_r[i]));
    }
}
