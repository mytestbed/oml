/*
 * Copyright 2007-2011 National ICT Australia (NICTA), Australia
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <check.h>
#include "oml2/omlc.h"
#include "filter/factory.h"
#include "filter/average_filter.h"
#include "filter/first_filter.h"
#include "filter/histogram_filter.h"
#include "filter/stddev_filter.h"
#include "oml2/oml_writer.h"
#include "check_util.h"

typedef struct _omlAvgFilterInstanceData AvgInstanceData;
typedef struct _omlFirstFilterInstanceData FirstInstanceData;
typedef struct _omlHistFilterInstanceData HistInstanceData;
typedef struct _omlStddevFilterInstanceData StddevInstanceData;


/* Fixtures */

void
filter_setup (void)
{
  register_builtin_filters ();
}

void
filter_teardown (void)
{
  /*
   * Each test case runs in a different process, so no need to tear
   * down the filter factory, but probably should include it at some point.
   */
}

/********************************************************************************/
/*                         GENERAL FILTER TESTS                                 */
/********************************************************************************/

START_TEST (test_filter_create)
{
  /*
   * Create an averaging filter and check that its core (generic) data
   * structure was correctly initialized.
   */
  OmlFilter* f = NULL;
  OmlFilterDef* def = NULL;

  // TBD:  turn this into a loop with a check for every different type of filter.
  f = create_filter ("avg", "inst", OML_LONG_VALUE, 2);

  fail_if (f == NULL, "Filter creation failed for averaging filter");

  fail_unless (strcmp (f->name, "inst") == 0, "Filter name is incorrect (%s), should be \"inst\"", f->name);
  fail_unless (f->output_count == 3, "Filter output width is incorrect (%d), should be 3: avg, min, max", f->output_count);
  fail_if (f->set == NULL, "Filter set function is NULL");
  fail_if (f->input == NULL, "Filter input function is NULL");
  fail_if (f->output == NULL, "Filter output function is NULL");
  fail_if (f->meta == NULL, "Filter meta function is NULL");
  fail_if (f->definition == NULL, "Filter definition is NULL");
  fail_if (f->instance_data == NULL, "Filter instance data is NULL");

  def = f->definition;

  fail_unless (strcmp (def[0].name, "avg") == 0, "Filter definition error: def[0].name = %s, should be \"avg\"", def[0].name);
  fail_unless (strcmp (def[1].name, "min") == 0, "Filter definition error: def[1].name = %s, should be \"min\"", def[1].name);
  fail_unless (strcmp (def[2].name, "max") == 0, "Filter definition error: def[2].name = %s, should be \"max\"", def[2].name);

  fail_unless (def[0].type == OML_DOUBLE_VALUE, "Filter definition error: def[0].type = %s, should be OML_DOUBLE_VALUE", def[0].type);
  fail_unless (def[1].type == OML_DOUBLE_VALUE, "Filter definition error: def[1].type = %s, should be OML_DOUBLE_VALUE", def[1].type);
  fail_unless (def[2].type == OML_DOUBLE_VALUE, "Filter definition error: def[2].type = %s, should be OML_DOUBLE_VALUE", def[2].type);

  fail_unless (f->index == 2);
  fail_unless (f->input_type == OML_LONG_VALUE);

  fail_unless (destroy_filter(f) == 0);
}
END_TEST

/********************************************************************************/
/*                         AVERAGING FILTER TESTS                               */
/********************************************************************************/

START_TEST (test_filter_avg_create)
{
  /*
   * Create an averaging filter and check that it was correctly initialized.
   */
  OmlFilter* f = NULL;
  AvgInstanceData* data = NULL;

  f = create_filter ("avg", "inst", OML_LONG_VALUE, 2);

  fail_if (f == NULL, "Filter creation failed for `avg' filter");
  fail_if (f->instance_data == NULL, "Filter instance data is NULL");

  fail_unless (f->index == 2);
  fail_unless (f->input_type == OML_LONG_VALUE);

  data = (AvgInstanceData*)f->instance_data;

  // Sample count and accumulator should be 0; min and max should be v. -ve and v. +ve respectively.
  fail_unless (data->sample_sum == 0);
  fail_unless (data->sample_count == 0);
  fail_unless (data->sample_min == HUGE, "Initial min val should be %f, but actually was %f", HUGE, data->sample_min);
  fail_unless (data->sample_max == -HUGE, "Initial min val should be %f, but actually was %f", -HUGE, data->sample_max);

  fail_unless (destroy_filter(f) == 0);
}
END_TEST

START_TEST (test_filter_avg_output)
{
  /*
   * Create an averaging filter and check that it was correctly initialized.
   */
  OmlFilter* f = NULL;
  AvgInstanceData* instdata = NULL;

  f = create_filter ("avg", "inst", OML_LONG_VALUE, 2);

  instdata = (AvgInstanceData*)f->instance_data;

  long input [] = { 1, 2, 3, 4, 5, 6 };
  double output [] = { 3.5, 1, 6 };

  TestVector** v_input = (TestVector**)malloc(1 * sizeof(TestVector));
  TestVector** v_output = (TestVector**)malloc(1 * sizeof(TestVector));

  v_input[0] = make_test_vector (input, OML_LONG_VALUE, 6);
  v_output[0] = make_test_vector (output, OML_DOUBLE_VALUE, 3);

  TestData* data = (TestData*) malloc (sizeof(TestData));
  data->count = 1;
  data->inputs = v_input;
  data->outputs = v_output;

  run_filter_test (data, f);

  // Sample count and accumulator should be 0; min and max should be v. -ve and v. +ve respectively.
  fail_unless (instdata->sample_sum == 0);
  fail_unless (instdata->sample_count == 0);
  fail_unless (instdata->sample_min == HUGE, "Initial min val should be %f, but actually was %f", HUGE, instdata->sample_min);
  fail_unless (instdata->sample_max == -HUGE, "Initial min val should be %f, but actually was %f", -HUGE, instdata->sample_max);

  fail_unless (destroy_filter(f) == 0);
}
END_TEST
/********************************************************************************/
/*                         'FIRST' FILTER TESTS                                 */
/********************************************************************************/

START_TEST (test_filter_first_create)
{
  /*
   * Create a "First" filter and check that it was correctly initialized.
   */
  OmlFilter* f = NULL;
  FirstInstanceData* data = NULL;

  f = create_filter ("first", "inst", OML_LONG_VALUE, 2);
  fail_if (f == NULL, "Filter creation failed for `first' filter");
  fail_if (f->instance_data == NULL, "Filter instance data is NULL");

  fail_unless (f->index == 2);
  fail_unless (f->input_type == OML_LONG_VALUE);

  data = (FirstInstanceData*)f->instance_data;

  // Sample count and accumulator should be 0; min and max should be v. -ve and v. +ve respectively.
  fail_unless (data->is_first == 1);

  fail_unless (destroy_filter(f) == 0);
}
END_TEST

/********************************************************************************/
/*                        HISTOGRAM FILTER TESTS                                */
/********************************************************************************/


START_TEST (test_filter_hist_create)
{
  /*
   * Create an histogram filter and check that it was correctly initialized.
   */
  OmlFilter* f = NULL;
  HistInstanceData* data = NULL;

  f = create_filter ("histogram", "inst", OML_LONG_VALUE, 2);

  fail_if (f == NULL, "Filter creation failed for `histogram' filter");
  fail_if (f->instance_data == NULL, "Filter instance data is NULL");

  fail_unless (f->index == 2);
  fail_unless (f->input_type == OML_LONG_VALUE);

  data = (HistInstanceData*)f->instance_data;

  /* Sample count and accumulator should be 0; min and max should be v. +ve and v. -ve respectively. */
  fail_unless (data->sample_sum == 0);
  fail_unless (data->sample_count == 0);
  fail_unless (data->sample_min == HUGE, "Initial min val should be %f, but actually was %f", HUGE, data->sample_min);
  fail_unless (data->sample_max == -HUGE, "Initial min val should be %f, but actually was %f", -HUGE, data->sample_max);

  fail_unless (destroy_filter(f) == 0);
}
END_TEST

/********************************************************************************/
/*                          STDDEV FILTER TESTS                                 */
/********************************************************************************/

START_TEST(test_filter_stddev_create)
{
  /*
   * Create an histogram filter and check that it was correctly initialized.
   */
  OmlFilter* f = NULL;
  StddevInstanceData* data = NULL;

  f = create_filter ("stddev", "inst", OML_LONG_VALUE, 2);

  fail_if (f == NULL, "Filter creation failed for `histogram' filter");
  fail_if (f->instance_data == NULL, "Filter instance data is NULL");

  fail_unless (f->index == 2);
  fail_unless (f->input_type == OML_LONG_VALUE);

  data = (StddevInstanceData*)f->instance_data;

  fail_unless (data->m == 0);
  fail_unless (data->s == 0);
  fail_unless (data->sample_count == 0);

  fail_unless (destroy_filter(f) == 0);
}
END_TEST


TestData*
stddev_0_data (void)
{
#include "stddev_0.c"
}

TestData*
stddev_1_data (void)
{
#include "stddev_1.c"
}

START_TEST (test_filter_stddev_0)
{
  TestData* test_data = stddev_0_data ();
  OmlFilter* f = create_filter ("stddev", "inst", OML_LONG_VALUE, 2);
  fail_if (f == NULL);
  StddevInstanceData* instance_data = (StddevInstanceData*)f->instance_data;
  fail_if (instance_data == NULL);


  run_filter_test (test_data, f);
}
END_TEST

START_TEST (test_filter_stddev_1)
{
  TestData* test_data = stddev_1_data ();
  OmlFilter* f = create_filter ("stddev", "inst", OML_LONG_VALUE, 2);
  fail_if (f == NULL);
  StddevInstanceData* instance_data = (StddevInstanceData*)f->instance_data;
  fail_if (instance_data == NULL);

  run_filter_test (test_data, f);
}
END_TEST

Suite*
filters_suite (void)
{
  Suite* s = suite_create ("Filters");

  /* Filter test cases */
  TCase* tc_filter = tcase_create ("FilterCore");
  TCase* tc_filter_avg = tcase_create ("FilterAverage");
  TCase* tc_filter_first = tcase_create ("FilterFirst");
  //  TCase* tc_filter_hist = tcase_create ("FilterHistogram");
  TCase* tc_filter_stddev = tcase_create ("FilterStddev");

  /* Setup fixtures */
  tcase_add_checked_fixture (tc_filter,       filter_setup, filter_teardown);
  tcase_add_checked_fixture (tc_filter_avg,   filter_setup, filter_teardown);
  tcase_add_checked_fixture (tc_filter_first, filter_setup, filter_teardown);
  //tcase_add_checked_fixture (tc_filter_hist,  filter_setup, filter_teardown);
  tcase_add_checked_fixture (tc_filter_stddev,filter_setup, filter_teardown);

  /* Add tests to test case "FilterCore" */
  tcase_add_test (tc_filter, test_filter_create);

  /* Add tests to test case "FilterAverage" */
  tcase_add_test (tc_filter_avg, test_filter_avg_create);
  tcase_add_test (tc_filter_avg, test_filter_avg_output);

  /* Add tests to test case "FilterFirst" */
  tcase_add_test (tc_filter_first, test_filter_first_create);

  /* Add tests to test case "FilterHistogram" */
  //tcase_add_test (tc_filter_hist, test_filter_hist_create);

  /* Add tests to test case "FilterStddev" */
  tcase_add_test (tc_filter_stddev, test_filter_stddev_create);
  tcase_add_test (tc_filter_stddev, test_filter_stddev_0);
  tcase_add_test (tc_filter_stddev, test_filter_stddev_1);

  /* Add the test cases to this test suite */
  suite_add_tcase (s, tc_filter);
  suite_add_tcase (s, tc_filter_avg);
  suite_add_tcase (s, tc_filter_first);
  //suite_add_tcase (s, tc_filter_hist);
  suite_add_tcase (s, tc_filter_stddev);

  return s;
}


/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
