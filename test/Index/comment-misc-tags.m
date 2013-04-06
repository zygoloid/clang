// RUN: rm -rf %t
// RUN: mkdir %t
// RUN: c-index-test -test-load-source all -comments-xml-schema=%S/../../bindings/xml/comment-xml-schema.rng %s > %t/out
// RUN: FileCheck %s < %t/out
// rdar://12379114

/*!
     @interface IOCommandGate
     @brief    This is a brief
     @abstract Single-threaded work-loop client request mechanism.
     @discussion An IOCommandGate instance is an extremely light weight mechanism that
         executes an action on the driver's work-loop...
     @textblock
       Many discussions about text
       Many1 discussions about text
       Many2 discussions about text
     @/textblock
 */
@interface IOCommandGate
@end

// CHECK:       (CXComment_BlockCommand CommandName=[abstract]
// CHECK-NEXT:    (CXComment_Paragraph
// CHECK-NEXT:       (CXComment_Text Text=[ Single-threaded work-loop client request mechanism.] HasTrailingNewline)
// CHECK:       (CXComment_BlockCommand CommandName=[discussion]
// CHECK-NEXT:     (CXComment_Paragraph
// CHECK-NEXT:       (CXComment_Text Text=[ An IOCommandGate instance is an extremely light weight mechanism that] HasTrailingNewline)
// CHECK-NEXT:       (CXComment_Text Text=[         executes an action on the driver's work-loop...] HasTrailingNewline)
// CHECK:       (CXComment_VerbatimBlockCommand CommandName=[textblock]
// CHECK-NEXT:     (CXComment_VerbatimBlockLine Text=[       Many discussions about text])
// CHECK-NEXT:       (CXComment_VerbatimBlockLine Text=[       Many1 discussions about text])
// CHECK-NEXT:       (CXComment_VerbatimBlockLine Text=[       Many2 discussions about text]))
// CHECK-NEXT:       (CXComment_Paragraph IsWhitespace

