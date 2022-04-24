struct stat;
struct rtcdate;

// system calls
int fork(void);
int exit(int) __attribute__((noreturn));
int wait(int*);
int pipe(int*);
int write(int, const void*, int);
int seek(int, int, const void*);
int read(int, void*, int);
int close(int);
int kill(int);
int exec(char*, char**);
int open(const char*, int);
int mknod(const char*, short, short);
int unlink(const char*);
int fstat(int fd, struct stat*);
int link(const char*, const char*);
int mkdir(const char*);
int chdir(const char*);
int dup(int);
int getpid(void);
char* sbrk(int);
int sleep(int);
int uptime(void);

// ulib.c
int stat(const char*, struct stat*);
char* strcpy(char*, const char*);
void *memmove(void*, const void*, int);
char* strchr(const char*, char c);
int strcmp(const char*, const char*);
void fprintf(int, const char*, ...);
void printf(const char*, ...);
char* gets(char*, int max);
uint strlen(const char*);
void* memset(void*, int, uint);
void* malloc(uint);
void free(void*);
int atoi(const char*);
int memcmp(const void *, const void *, uint);
void *memcpy(void *, const void *, uint);

// uwindow.c
typedef struct window *window_handle; // opaque type
struct window_dim
{
    uint16 width;
    uint16 height;
};
struct windowevent;

window_handle window_create();
void window_destroy(window_handle win);
struct window_dim window_getdimensions(window_handle win);
int window_pollevent(window_handle win, struct windowevent *evts, int max_evts);
void window_drawsprite(window_handle win, uint x, uint y, uint w, uint h, uint8 *data);
void window_drawrect(window_handle win, uint x, uint y, uint w, uint h, uint8 color);
void window_drawchar(window_handle win, uint x, uint y, char c, uint8 color);
void window_drawline(window_handle win, uint x1, uint y1, uint x2, uint y2, uint8 color);
void window_drawcircle(window_handle win, uint x, uint y, uint r, uint8 color);
