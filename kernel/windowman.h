struct windowevent {
    enum {EVT_KEY, HEAD, TAIL} type;
    int payload;
};