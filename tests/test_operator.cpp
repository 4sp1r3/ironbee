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
/// @brief IronBee --- Operator Test Functions
///
/// @author Craig Forbes <cforbes@qualys.com>
//////////////////////////////////////////////////////////////////////////////

#include "base_fixture.h"
#include <ironbee/operator.h>
#include <ironbee/server.h>
#include <ironbee/engine.h>
#include <ironbee/mpool.h>
#include <ironbee/field.h>
#include "gtest/gtest.h"


ib_status_t test_create_fn(
    ib_engine_t        *ib,
    ib_context_t       *ctx,
    ib_mpool_t         *pool,
    const char         *data,
    ib_operator_inst_t *op_inst,
    void               *cbdata
)
{
    char *str;

    if (strcmp(data, "INVALID") == 0) {
        return IB_EINVAL;
    }
    str = ib_mpool_strdup(pool, data);
    if (str == NULL) {
        return IB_EALLOC;
    }

    op_inst->data = str;

    return IB_OK;
}

ib_status_t test_destroy_fn(
    ib_operator_inst_t *op_inst,
    void               *cbdata
)
{
    return IB_OK;
}

ib_status_t test_execute_fn(
    ib_tx_t    *tx,
    void       *data,
    ib_flags_t  flags,
    ib_field_t *field,
    ib_field_t *capture,
    ib_num_t   *result,
    void       *cbdata
)
{
    char *searchstr = (char *)data;
    const char* s;
    ib_status_t rc;

    if (field->type != IB_FTYPE_NULSTR) {
        return IB_EINVAL;
    }

    rc = ib_field_value(field, ib_ftype_nulstr_out(&s));
    if (rc != IB_OK) {
        return rc;
    }

    if (strstr(s, searchstr) == NULL) {
        *result = 0;
    }
    else {
        *result = 1;
    }

    return IB_OK;
}

class OperatorTest : public BaseTransactionFixture
{
    void SetUp()
    {
        BaseFixture::SetUp();
    }
};

TEST_F(OperatorTest, OperatorCallTest)
{
    ib_status_t status;
    ib_num_t call_result;
    ib_operator_inst_t *op;

    status = ib_operator_register(ib_engine,
                                  "test_op",
                                  IB_OP_CAPABILITY_NON_STREAM,
                                  test_create_fn,
                                  NULL,
                                  test_destroy_fn,
                                  NULL,
                                  test_execute_fn,
                                  NULL);
    ASSERT_EQ(IB_OK, status);

    status = ib_operator_inst_create(ib_engine,
                                     NULL,
                                     IB_OP_CAPABILITY_NON_STREAM,
                                     "test_op",
                                     "INVALID",
                                     IB_OPINST_FLAG_NONE,
                                     &op);
    ASSERT_EQ(IB_EINVAL, status);

    status = ib_operator_inst_create(ib_engine,
                                     NULL,
                                     IB_OP_CAPABILITY_NON_STREAM,
                                     "test_op",
                                     "data",
                                     IB_OPINST_FLAG_NONE,
                                     &op);
    ASSERT_EQ(IB_OK, status);


    ib_field_t *field;
    const char *matching = "data matching string";
    const char *nonmatching = "non matching string";
    ib_field_create(
        &field,
        ib_engine_pool_main_get(ib_engine),
        IB_FIELD_NAME("testfield"),
        IB_FTYPE_NULSTR,
        NULL
    );

    ib_field_setv(field, ib_ftype_nulstr_in(matching));
    status = ib_operator_execute(ib_tx, op, field, NULL, &call_result);
    ASSERT_EQ(IB_OK, status);
    EXPECT_EQ(1, call_result);

    ib_field_setv(field, ib_ftype_nulstr_in(nonmatching));
    status = ib_operator_execute(ib_tx, op, field, NULL, &call_result);
    ASSERT_EQ(IB_OK, status);
    EXPECT_EQ(0, call_result);

    status = ib_operator_inst_destroy(op);
    ASSERT_EQ(IB_OK, status);
}


class CoreOperatorsTest : public BaseTransactionFixture
{
    void SetUp()
    {
        BaseFixture::SetUp();
        configureIronBee();
        performTx();
    }
};

TEST_F(CoreOperatorsTest, ContainsTest)
{
    ib_status_t status;
    ib_num_t call_result;
    ib_operator_inst_t *op;

    status = ib_operator_inst_create(ib_engine,
                                     NULL,
                                     IB_OP_CAPABILITY_NON_STREAM,
                                     "contains",
                                     "needle",
                                     IB_OPINST_FLAG_NONE,
                                     &op);
    ASSERT_EQ(IB_OK, status);

    // call contains
    ib_field_t *field;
    const char *matching = "data with needle in it";
    const char *nonmatching = "non matching string";
    ib_field_create(
        &field,
        ib_engine_pool_main_get(ib_engine),
        IB_FIELD_NAME("testfield"),
        IB_FTYPE_NULSTR,
        NULL
    );

    ib_field_setv(field, ib_ftype_nulstr_in(matching));
    status = ib_operator_execute(ib_tx, op, field, NULL, &call_result);
    ASSERT_EQ(IB_OK, status);
    EXPECT_EQ(1, call_result);

    ib_field_setv(field, ib_ftype_nulstr_in(nonmatching));
    status = ib_operator_execute(ib_tx, op, field, NULL, &call_result);
    ASSERT_EQ(IB_OK, status);
    EXPECT_EQ(0, call_result);
}

TEST_F(CoreOperatorsTest, EqTest)
{
    ib_status_t status;
    ib_num_t call_result;
    ib_operator_inst_t *op;

    status = ib_operator_inst_create(ib_engine,
                                     NULL,
                                     IB_OP_CAPABILITY_NON_STREAM,
                                     "eq",
                                     "1",
                                     IB_OPINST_FLAG_NONE,
                                     &op);
    ASSERT_EQ(IB_OK, status);

    // call contains
    ib_field_t *field;
    const ib_num_t matching = 1;
    const ib_num_t nonmatching = 2;
    ib_field_create(
        &field,
        ib_engine_pool_main_get(ib_engine),
        IB_FIELD_NAME("testfield"),
        IB_FTYPE_NUM,
        ib_ftype_num_in(&matching)
    );

    ib_field_setv(field, ib_ftype_num_in(&matching));
    status = ib_operator_execute(ib_tx, op, field, NULL, &call_result);
    ASSERT_EQ(IB_OK, status);
    EXPECT_EQ(1, call_result);

    ib_field_setv(field, ib_ftype_num_in(&nonmatching));
    status = ib_operator_execute(ib_tx, op, field, NULL, &call_result);
    ASSERT_EQ(IB_OK, status);
    EXPECT_EQ(0, call_result);
}

TEST_F(CoreOperatorsTest, NeTest)
{
    ib_status_t status;
    ib_num_t call_result;
    ib_operator_inst_t *op;

    status = ib_operator_inst_create(ib_engine,
                                     NULL,
                                     IB_OP_CAPABILITY_NON_STREAM,
                                     "ne",
                                     "1",
                                     IB_OPINST_FLAG_NONE,
                                     &op);
    ASSERT_EQ(IB_OK, status);

    // call contains
    ib_field_t *field;
    const ib_num_t matching = 2;
    const ib_num_t nonmatching = 1;
    ib_field_create(
        &field,
        ib_engine_pool_main_get(ib_engine),
        IB_FIELD_NAME("testfield"),
        IB_FTYPE_NUM,
        ib_ftype_num_in(&matching)
    );

    ib_field_setv(field, ib_ftype_num_in(&matching));
    status = ib_operator_execute(ib_tx, op, field, NULL, &call_result);
    ASSERT_EQ(IB_OK, status);
    EXPECT_EQ(1, call_result);

    ib_field_setv(field, ib_ftype_num_in(&nonmatching));
    status = ib_operator_execute(ib_tx, op, field, NULL, &call_result);
    ASSERT_EQ(IB_OK, status);
    EXPECT_EQ(0, call_result);
}
