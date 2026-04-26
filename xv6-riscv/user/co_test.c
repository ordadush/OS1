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
      int value = co_yield(pid1, 100);
      printf("Child received: %d\n", value);
    }
    exit(0);
  } else {
    // Parent process
    for (int i = 0; i < 3; i++) {
      int value = co_yield(pid2, 200);
      printf("Parent received: %d\n", value);
    }
    // === THE FIX: Wake up the sleeping child so it can exit ===
    kill(pid2); 
    wait(0);
  }
  printf("Test 1 PASSED\n\n");
}

// Test 2: Error case - non-existent PID
void test_nonexistent_pid(void) {
  printf("=== Test 2: Non-existent PID ===\n");
  int result = co_yield(99999, 42);
  if (result != -1) {
    printf("ERROR: Expected -1, got %d\n", result);
    exit(1);
  }
  printf("Test 2 PASSED\n\n");
}

// Test 3: Error case - self-yield
void test_self_yield(void) {
  printf("=== Test 3: Self-yield (error case) ===\n");
  int result = co_yield(getpid(), 42);
  if (result != -1) {
    printf("ERROR: Expected -1, got %d\n", result);
    exit(1);
  }
  printf("Test 3 PASSED\n\n");
}

// Test 4: Error case - negative PID
void test_negative_pid(void) {
  printf("=== Test 4: Negative PID (error case) ===\n");
  int result = co_yield(-1, 42);
  if (result != -1) {
    printf("ERROR: Expected -1, got %d\n", result);
    exit(1);
  }
  printf("Test 4 PASSED\n\n");
}

// Test 5: Error case - zero PID
void test_zero_pid(void) {
  printf("=== Test 5: Zero PID (error case) ===\n");
  int result = co_yield(0, 42);
  if (result != -1) {
    printf("ERROR: Expected -1, got %d\n", result);
    exit(1);
  }
  printf("Test 5 PASSED\n\n");
}

// Test 6: Error case - killed process
void test_killed_process(void) {
  printf("=== Test 6: Yield to killed process ===\n");
  int pid = fork();
  if (pid == 0) {
    exit(0);
  } else {
    wait(0);
    int result = co_yield(pid, 42);
    if (result != -1) {
      printf("ERROR: Expected -1, got %d\n", result);
      exit(1);
    }
  }
  printf("Test 6 PASSED\n\n");
}

// Test 7: Multiple rounds of yielding
void test_multiple_rounds(void) {
  printf("=== Test 7: Multiple Rounds of Yielding ===\n");
  
  int pid1 = getpid();
  int pid2 = fork();
  
  if (pid2 == 0) {
    for (int i = 0; i < 5; i++) {
      co_yield(pid1, 1000);
    }
    exit(0);
  } else {
    for (int i = 0; i < 5; i++) {
      co_yield(pid2, 2000);
    }
    // === THE FIX: Wake up the sleeping child ===
    kill(pid2);
    wait(0);
  }
  printf("Test 7 PASSED\n\n");
}

// Test 8: Verify values are passed correctly (Large values edge cases)
void test_value_passing(void) {
  printf("=== Test 8: Value Passing (Large Numbers) ===\n");
  
  int pid1 = getpid();
  int pid2 = fork();
  
  if (pid2 == 0) {
    // Child process
    for (int i = 0; i < 3; i++) {
      int value = co_yield(pid1, 999999);
      if (value != 32767) {
        printf("ERROR: Child expected 32767, got %d\n", value);
        exit(1);
      }
    }
    exit(0);
  } else {
    // Parent process
    for (int i = 0; i < 3; i++) {
      int value = co_yield(pid2, 32767);
      if (value != 999999) {
        printf("ERROR: Parent expected 999999, got %d\n", value);
        exit(1);
      }
    }
    // Wake up the sleeping child
    kill(pid2);
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