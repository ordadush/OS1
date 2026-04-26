#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// Test 1: Basic coroutine yielding between parent and child
void test_basic_yield(void) {
  printf("=== Test 1: Basic Coroutine Yield ===\n");
  
  int pid1 = getpid(); // Parent PID
  int pid2 = fork();   // Child PID
  
  if (pid2 == 0) {
    // Child process
    for (int i = 0; i < 3; i++) {
      int value = co_yield(pid1, 100 + i);
      printf("Child received: %d\n", value);
      if (value != 200 + i) {
        printf("ERROR: Child expected %d but got %d\n", 200 + i, value);
        exit(1);
      }
    }
    exit(0);
  } else {
    // Parent process
    for (int i = 0; i < 3; i++) {
      int value = co_yield(pid2, 200 + i);
      printf("Parent received: %d\n", value);
      if (value != 100 + i) {
        printf("ERROR: Parent expected %d but got %d\n", 100 + i, value);
        exit(1);
      }
    }
    wait(0);
  }
  printf("Test 1 PASSED\n\n");
}

// Test 2: Error case - non-existent PID
void test_nonexistent_pid(void) {
  printf("=== Test 2: Non-existent PID ===\n");
  
  int result = co_yield(99999, 42);
  if (result != -1) {
    printf("ERROR: Expected -1 for non-existent PID, got %d\n", result);
    exit(1);
  }
  printf("Correctly returned -1 for non-existent PID\n");
  printf("Test 2 PASSED\n\n");
}

// Test 3: Error case - self-yield
void test_self_yield(void) {
  printf("=== Test 3: Self-yield (error case) ===\n");
  
  int result = co_yield(getpid(), 42);
  if (result != -1) {
    printf("ERROR: Expected -1 for self-yield, got %d\n", result);
    exit(1);
  }
  printf("Correctly returned -1 for self-yield\n");
  printf("Test 3 PASSED\n\n");
}

// Test 4: Error case - negative PID
void test_negative_pid(void) {
  printf("=== Test 4: Negative PID (error case) ===\n");
  
  int result = co_yield(-1, 42);
  if (result != -1) {
    printf("ERROR: Expected -1 for negative PID, got %d\n", result);
    exit(1);
  }
  printf("Correctly returned -1 for negative PID\n");
  printf("Test 4 PASSED\n\n");
}

// Test 5: Error case - zero PID
void test_zero_pid(void) {
  printf("=== Test 5: Zero PID (error case) ===\n");
  
  int result = co_yield(0, 42);
  if (result != -1) {
    printf("ERROR: Expected -1 for zero PID, got %d\n", result);
    exit(1);
  }
  printf("Correctly returned -1 for zero PID\n");
  printf("Test 5 PASSED\n\n");
}

// Test 6: Error case - killed process
void test_killed_process(void) {
  printf("=== Test 6: Yield to killed process ===\n");
  
  int pid = fork();
  if (pid == 0) {
    // Child exits immediately
    exit(0);
  } else {
    // Parent waits for child to exit
    wait(0);
    
    // Now try to yield to the killed child
    int result = co_yield(pid, 42);
    if (result != -1) {
      printf("ERROR: Expected -1 for killed process, got %d\n", result);
      exit(1);
    }
    printf("Correctly returned -1 for killed process\n");
  }
  printf("Test 6 PASSED\n\n");
}

// Test 7: Multiple rounds of yielding
void test_multiple_rounds(void) {
  printf("=== Test 7: Multiple Rounds of Yielding ===\n");
  
  int pid1 = getpid();
  int pid2 = fork();
  
  if (pid2 == 0) {
    // Child
    for (int i = 0; i < 5; i++) {
      int value = co_yield(pid1, 1000 + i);
      if (value != 2000 + i) {
        printf("ERROR: Child round %d: expected %d, got %d\n", i, 2000 + i, value);
        exit(1);
      }
    }
    exit(0);
  } else {
    // Parent
    for (int i = 0; i < 5; i++) {
      int value = co_yield(pid2, 2000 + i);
      if (value != 1000 + i) {
        printf("ERROR: Parent round %d: expected %d, got %d\n", i, 1000 + i, value);
        exit(1);
      }
    }
    wait(0);
  }
  printf("Test 7 PASSED\n\n");
}

// Test 8: Verify values are passed correctly (including negatives and large values)
void test_value_passing(void) {
  printf("=== Test 8: Value Passing (including edge cases) ===\n");
  
  int pid1 = getpid();
  int pid2 = fork();
  
  if (pid2 == 0) {
    // Child: test various values
    int vals[] = {3, 1, 100, 32767};
    for (int i = 0; i < 4; i++) {
      int value = co_yield(pid1, vals[i]);
      // Parent should echo back
      if (value != vals[i]) {
        printf("ERROR: Child round %d: expected %d, got %d\n", i, vals[i], value);
        exit(1);
      }
    }
    exit(0);
  } else {
    // Parent: echo back the same values
    int vals[] = {3, 1, 100, 32767};
    for (int i = 0; i < 4; i++) {
      int value = co_yield(pid2, vals[i]);
      // Should receive what child sent
      if (value != vals[i]) {
        printf("ERROR: Parent round %d: expected %d, got %d\n", i, vals[i], value);
        exit(1);
      }
    }
    wait(0);
  }
  printf("Test 8 PASSED\n\n");
}

int main(void) {
  printf("Starting Co-yield Syscall Tests\n\n");
  
  test_basic_yield();
  test_nonexistent_pid();
  test_self_yield();
  test_negative_pid();
  test_zero_pid();
  test_killed_process();
  test_multiple_rounds();
  test_value_passing();
  
  printf("=== ALL TESTS PASSED ===\n");
  
  exit(0);
}
