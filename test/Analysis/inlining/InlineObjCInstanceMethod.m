// RUN: %clang --analyze -Xanalyzer -analyzer-checker=core -Xclang -verify %s

#include "InlineObjCInstanceMethod.h"

// Method is defined in the parent; called through self.
@interface MyParent : NSObject
- (int)getInt;
@end
@implementation MyParent
- (int)getInt {
    return 0;
}
@end

@interface MyClass : MyParent
@end
@implementation MyClass
- (int)testDynDispatchSelf {
  int y = [self getInt];
  return 5/y; // expected-warning {{Division by zero}}
}

// Get the dynamic type info from a cast (from id to MyClass*).
+ (int)testAllocInit {
  MyClass *a = [[self alloc] init];
  return 5/[a getInt]; // expected-warning {{Division by zero}}
}

// Method is called on inited object.
+ (int)testAllocInit2 {
  MyClass *a = [[MyClass alloc] init];
  return 5/[a getInt]; // expected-warning {{Division by zero}}
}

// Method is called on a parameter.
+ (int)testParam: (MyClass*) a {
  return 5/[a getInt]; // expected-warning {{Division by zero}}
}

// Method is called on a parameter of unnown type.
+ (int)testParamUnknownType: (id) a {
  return 5/[a getInt]; // no warning
}

@end

// TODO: When method is inlined, the attribute reset should be visible.
@interface TestSettingAnAttributeInCallee : NSObject {
  int _attribute;
}
  - (void) method2;
@end

@implementation TestSettingAnAttributeInCallee
- (int) method1 {
  [self method2];
  return 5/_attribute; // expected-warning {{Division by zero}}
}

- (void) method2 {
  _attribute = 0;
}
@end

@interface TestSettingAnAttributeInCaller : NSObject {
  int _attribute;
}
  - (int) method2;
@end

@implementation TestSettingAnAttributeInCaller
- (void) method1 {
  _attribute = 0;
  [self method2];
}

- (int) method2 {
  return 5/_attribute; // expected-warning {{Division by zero}}
}
@end


// Don't crash if we don't know the receiver's region.
void randomlyMessageAnObject(MyClass *arr[], int i) {
  (void)[arr[i] getInt];
}