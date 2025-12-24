#pragma once

struct String_Builder_Node {
    String_Builder_Node *next = null;
    char *data = null;
    u64   size = 0;
};

// Convenient multi-purpose string builder, uses local scratch as memory storage by default,
// so keep in mind builder lifetime in this case.
struct String_Builder {
    Allocator allocator = context.allocator;
    String_Builder_Node *node = null;
};

void   append            (String_Builder &builder, const void *data, u64 size);
void   append            (String_Builder &builder, const char *s);
void   append            (String_Builder &builder, const char *s, u64 count);
void   append            (String_Builder &builder, char c);
void   append            (String_Builder &builder, Buffer buffer);
void   append            (String_Builder &builder, String string);
void   print_to_builder  (String_Builder &builder, const char *format, ...);
void   print_to_builder  (String_Builder &builder, const char *format, va_list args);
String builder_to_string (String_Builder &builder);
String builder_to_string (String_Builder &builder, Allocator allocator);

template <typename T>
void put(String_Builder &builder, const T &data) {
    append(builder, &data, sizeof(data));
}
