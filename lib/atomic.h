#ifndef _ATOMIC_H
#define _ATOMIC_H

/* Check GCC version, just to be safe */
#if !defined(__GNUC__) || (__GNUC__ < 4) || (__GNUC_MINOR__ < 1)
# error atomic.h works only with GCC newer than version 4.1
#endif /* GNUC >= 4.1 */

/**
 * Atomic type.
 */
typedef volatile int atomic_t;
typedef volatile int spin_t;

/**
 * Read atomic variable
 * @param v variable of type atomic_t
 *
 * Atomically reads the value of @v.
 */
#define atomic_read(v) (*v)

/**
 * Set atomic variable
 * @param i value to be set
 * @param v variable of type atomic_t
 */
static inline void atomic_set( int i, atomic_t *v ) {
	*v = i;
}

static inline int atomic_sub_and_test( atomic_t *v )
{
	return !(__sync_sub_and_fetch(v, 1));
}

/**
 * @brief Increment and test
 * @param v pointer of type atomic_t
 *
 * Atomically increments @v by 1
 * and returns true if the result is zero, or false for all
 * other cases.
 */
static inline int atomic_inc_and_test( atomic_t *v )
{
	return !(__sync_add_and_fetch(v, 1));
}

static inline int atomic_inc_read( atomic_t *v )
{
	return (__sync_add_and_fetch(v, 1));
}

/**
 * @brief Atomic add
 * @param v pointer of type atomic_t
 * @param i integer value to add
 *
 * Atomically adds @i to @v
 */
static inline void atomic_add( int i, atomic_t *v )
{
	(void)__sync_add_and_fetch(v, i);
}

/**
 * @brief Initializes spin lock
 * @lock pointer to the lock, of type spin_t
 *
 * Initializes @lock. Assumes a free lock has a 0 value.
 */
static inline void spin_lock_init( spin_t *lock )
{
	*lock = 0;
}

/**
 * @brief Acquires spin lock
 * @lock pointer to the lock, of type spin_t
 *
 * Acquires @lock. Spins till successful. 
 */
static inline void spin_lock( spin_t *lock )
{
	do {
		while ( *lock ){};
	} while ( __sync_lock_test_and_set( lock, 1 ) );
}

/**
 * @brief Releases spin lock
 * @lock pointer to the lock, of type spin_t
 *
 * Releases @lock.
 */
static inline void spin_unlock( spin_t *lock ) {
	__sync_lock_release( lock );
}

static inline int compare_and_swap(void **ptr,void *old_ptr,void *new_ptr) {
	return __sync_bool_compare_and_swap(ptr,old_ptr,new_ptr);
}
#endif
