#!/usr/bin/env python

import math
import sys
import types

def vector_type(v):
    tv = [ type(x) for x in v ]

    if types.FloatType in tv:
        return types.FloatType
    elif types.IntType in tv:
        return types.IntType

def type_strings(t):
    if t == types.FloatType:
        return ("double", "OML_DOUBLE_VALUE")
    elif t == types.IntType:
        return ("long", "OML_LONG_VALUE")
    else:
        return ("int", "OML_BAD_VALUE")

def print_cvec(strm, v):

    strm.write('{ ')
    i = 0
    for x in v:
        print >>strm,x,
        if (i != len(v)-1):
            strm.write (', ')
        i = i + 1

    strm.write(' }')

def cvec_string(v):
    result = ''
    result = result + '{ '
    i = 0
    for x in v:
        result = result + str(x)
        if (i != len(v)-1):
            result = result + ', '
        i = i + 1

    result = result + ' }'

    return result

def stddev(v):
    '''
    Compute standard deviation for list v.
    '''
    mean = float(sum(v)) / len(v)

    y = sum([ (x - mean) * (x - mean) for x in v ]) / (len(v) - 1)

    return (math.sqrt(y), y)

def print_test(strm, t, name):

    i = 0;
    inputs = []
    inputs_tv = []
    for input in t['inputs']:
        (tt, oml_tt) = type_strings(vector_type(input))
        inputs.append('%s %s_input_%d [] = %s;' % (tt, name, i, cvec_string(input)));
        inputs_tv.append('%s_tv_input[%d] = make_test_vector (%s_input_%d, %s, %d);' % (name, i, name, i, oml_tt, len(input)))
        i = i + 1

    input_count = i

    i = 0
    outputs = []
    outputs_tv = []
    for output in t['outputs']:
        (tt, oml_tt) = type_strings(vector_type(output))
        outputs.append('%s %s_output_%d [] = %s;' % (tt, name, i, cvec_string(output)))
        outputs_tv.append('%s_tv_output[%d] = make_test_vector (%s_output_%d, %s, %d);' % (name, i, name, i, oml_tt, len(output)))
        i = i + 1

    output_count = i

    strm.write('\n'.join(inputs));     strm.write('\n')
    strm.write('\n'.join(outputs));    strm.write('\n\n')

    strm.write('TestVector** %s_tv_input  = (TestVector**)malloc(%s * sizeof(TestVector*));\n' % (name, input_count))
    strm.write('if (!%s_tv_input) return NULL;\n' % name)
    strm.write('TestVector** %s_tv_output = (TestVector**)malloc(%s * sizeof(TestVector*));\n' % (name, output_count))
    strm.write('if (!%s_tv_output)\n{\n free(%s_tv_input);\n return NULL;\n}\n' % (name, name))

    strm.write('\n'.join(inputs_tv));  strm.write('\n')
    strm.write('\n'.join(outputs_tv)); strm.write('\n\n')

    strm.write('TestData* %s_data = (TestData*)malloc(sizeof(TestData));\n' % name)
    strm.write('if (!%s_data)\n{\n free(%s_tv_input); free(%s_tv_output);\n return NULL;\n}\n' % (name, name, name))

    strm.write('%s_data->count = %d;\n' % (name, input_count))
    strm.write('%s_data->inputs = %s_tv_input;\n' % (name, name))
    strm.write('%s_data->outputs = %s_tv_output;\n' % (name, name))
    strm.write('return %s_data;\n' % name)

def main():


    tv0 = { 'inputs'  : [[ 1, 2, 3 ]] }
    tv1 = { 'inputs'  : [[ 1, 2, 3 ],
                         [ 2, 3, 4 ],
                         [ -7, -2, -11 ]] }

    test_list = [ tv0, tv1, ]

    for t in test_list:
        av = []
        t['outputs'] = []
        for v in t['inputs']:
            av = av + v
            (s, v) = stddev(av)
            t['outputs'].append([s, v])

    i = 0
    for t in test_list:
        name = "stddev_%d" % i
        f = open('%s.c' % name, 'w+')
        print_test(f, t, "stddev_%d" % i)
        i = i + 1


if __name__ == '__main__': main()
