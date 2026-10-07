// SPDX-License-Identifier: GPL`
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>

#include "tpool.h"

struct argument {
	unsigned long *true_count;
	unsigned long *false_count;
	unsigned long sample_size;
	unsigned int n;
	unsigned int seed;
};

/* generate a random floating point number from min to max */

int inside_Q(double x, double y)
{
	static const double a = -0.5, b = 1.0, c = -0.5, d = 1.0;

	return ((a <= x && x <= b) && (c <= y && y <= d));
}

static inline double randfrom(unsigned int *seed, double min, double max)
{
	return min + rand_r(seed) / ((double)RAND_MAX / (max - min));
}

static inline double distance(double x, double y)
{
	return sqrt((x * x) + (y * y));
}

unsigned int closest(double siteOpts[], unsigned int n)
{
	double d;
	int closestIndex = 0;

	double minD = distance(siteOpts[0], siteOpts[1]);

	for (unsigned int i = 1; i < n; i++) {
		d = distance(siteOpts[2 * i], siteOpts[2 * i + 1]);
		if (d < minD) {
			minD = d;
			closestIndex = i;
		}
	}

	return closestIndex;
}

void tess_test(void *arg)
{
	struct argument *val = arg;
	unsigned long sample_size = val->sample_size;
	unsigned int n = val->n;
	unsigned int seed = val->seed;

	// allocate all of our sites, we assume it works bcs lazy
	double *siteOpts = (double *)malloc(2 * n * sizeof(double));

	unsigned long *false_count = val->false_count;
	unsigned long *true_count = val->true_count;

	for (unsigned long i = 0; i < sample_size; i++) {
		// first construct all of our sites
		for (unsigned int j = 0; j < n; j++) {
			siteOpts[2 * j] = randfrom(&seed, -2, 2);
			siteOpts[2 * j + 1] = randfrom(&seed, -2, 2);
		}
		// now all of our sites are constructed, see if closest is correct
		unsigned int closestSite = closest(siteOpts, n);

		// see if it is indeed correct
		if (inside_Q(siteOpts[2 * closestSite],
			     siteOpts[2 * closestSite + 1]))
			(*true_count)++;
		else
			(*false_count)++;
	}
}

int main(int argc, char **argv)
{
	long sample_size = atol(argv[1]);
	const int num_threads = 4;
	unsigned int n = 2;

	int size_per_thread = sample_size / num_threads;

	srand(time(NULL));

	tpool *tm;
	struct argument *vals;

	tm = tpool_create(num_threads);
	vals = calloc(num_threads, sizeof(*vals));

	unsigned long *true_counts;
	unsigned long *false_counts;

	true_counts = malloc(num_threads * sizeof(unsigned long));
	false_counts = malloc(num_threads * sizeof(unsigned long));

	for (int i = 0; i < num_threads; i++) {
		vals[i] = (struct argument){
			.true_count = true_counts + i,
			.false_count = false_counts + i,
			.n = n,
			.sample_size = size_per_thread,
		};
		tpool_add_work(tm, tess_test, vals + i);
	}
	tpool_wait(tm);
	tpool_destroy(tm);
	free(vals);

	// print all
	unsigned long total_true = 0;
	unsigned long total_false = 0;

	for (int i = 0; i < num_threads; i++) {
		total_true += true_counts[i];
		total_false += false_counts[i];
	}

	printf("%f\t - %ld \t-\t %d * %d\n", (double)total_false / sample_size,
	       sample_size, size_per_thread, num_threads);

	free(true_counts);
	free(false_counts);

	return 0;
}
