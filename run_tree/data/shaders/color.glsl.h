vec4 rgba_unpack(uint rgba) {
    vec4 color;
    color.x = float((rgba & 0xFF000000) >> 24) / 255.0f;
    color.y = float((rgba & 0x00FF0000) >> 16) / 255.0f;
    color.z = float((rgba & 0x0000FF00) >> 8)  / 255.0f;
    color.w = float((rgba & 0x000000FF) >> 0)  / 255.0f;
    return color;
}
