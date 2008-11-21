#include <stdio.h>
#include <unistd.h>
#include <libmetrics.h>

#define NUM_TESTS 3 
#define SLEEP_TIME 5

int
main (void)
{
    g_val_t val;
    Metric *metric;
    int check, i;

    /* Initialize libmetrics */
    fprintf (stderr, "Initializing...");
    metric_init();
    fprintf (stderr, "\n");

    /* Loop through a few times to make sure everything works */
    for (check = 0; check < NUM_TESTS; check++) {
        fprintf(stderr,"============= Running test #%d of %d =================\n", check+1, NUM_TESTS);

        metric = first_metric();

        do {
            fprintf (stderr, "%20s = ", metric->name);
            val = metric->function();

            switch (metric->type) {
                case g_string:
                    fprintf (stderr, "%s (g_string)", val.str);
                    break;
                case g_int8:
                    fprintf (stderr, "%d (g_int8)", (int) val.int8);
                    break;
                case g_uint8:
                    fprintf (stderr, "%d (g_uint8)", (unsigned int) val.uint8);
                    break;
                case g_int16:
                    fprintf (stderr, "%d (g_int16)", (int) val.int16);
                    break;
                case g_uint16:
                    fprintf (stderr, "%d (g_uint16)", (unsigned int) val.uint16);
                    break;
                case g_int32:
                    fprintf (stderr, "%d (g_int32)", (int) val.int32);
                    break;
                case g_uint32:
                    fprintf (stderr, "%d (g_uint32)", (int) val.uint32);
                    break;
                case g_float:
                    fprintf (stderr, "%f (g_float)", val.f);
                    break;
                case g_double:
                    fprintf (stderr, "%f (g_double)", val.d);
                    break;
                case g_timestamp:
                    fprintf (stderr, "%d (g_timestamp)", val.uint32);
                    break;
            }

            fprintf (stderr, "\n");
        } while ((metric = next_metric(metric)) != NULL);


        if(check != (NUM_TESTS - 1)) {
            sleep(SLEEP_TIME);
        }
    }

    return 0;
}
