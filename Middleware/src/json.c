
#include "json.h"

#include "xprintf.h"



static char json_esc(int c, int esc) {
    const char *p, *e[] = {"\b\f\n\r\t\\\"", "bfnrt\\\""};
    const char *esc1 = esc ? e[0] : e[1], *esc2 = esc ? e[1] : e[0];
    for (p = esc1; *p != '\0'; p++) {
        if (*p == c) return esc2[p - esc1];
    }
    return 0;
}

static int json_pass_string(const char *s, int len) {
    int i;
    for (i = 0; i < len; i++) {
        if (s[i] == '\\' && i + 1 < len && json_esc(s[i + 1], 1)) {
            i++;
        } else if (s[i] == '\0') {
            return -1;
        } else if (s[i] == '"') {
            return i;
        }
    }
    return -1;
}

int json_get(const char *s, int len, const char *path, int *toklen) {
    enum { S_VALUE, S_KEY, S_COLON, S_COMMA_OR_EOO } expecting = S_VALUE;
    unsigned char nesting[20];
    int i = 0;             // Current offset in `s`
    int j = 0;             // Offset in `s` we're looking for (return value)
    int depth = 0;         // Current depth (nesting level)
    int ed = 0;            // Expected depth
    int pos = 1;           // Current position in `path`
    int ci = -1, ei = -1;  // Current and expected index in array

    if (toklen) *toklen = 0;
    if (path[0] != '$') return -1;

#define MG_CHECKRET()                                       \
    do {                                                    \
        if (depth == ed && path[pos] == '\0' && ci == ei) { \
            if (toklen) *toklen = i - j + 1;                \
            return j;                                       \
        }                                                   \
    } while (0)

// In the ascii table, the distance between `[` and `]` is 2.
// Ditto for `{` and `}`. Hence +2 in the code below.
#define MG_EOO()                                    \
    do {                                            \
        if (depth == ed && ci != ei) return -2;     \
        if (c != nesting[depth - 1] + 2) return -1; \
        depth--;                                    \
        MG_CHECKRET();                              \
    } while (0)

    for (i = 0; i < len; i++) {
        unsigned char c = ((unsigned char *)s)[i];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') continue;
        switch (expecting) {
            case S_VALUE:
                // p("V %s [%.*s] %d %d %d %d\n", path, pos, path, depth, ed, ci, ei);
                if (depth == ed) j = i;
                if (c == '{') {
                    if (depth >= (int)sizeof(nesting)) return -3;
                    if (depth == ed && path[pos] == '.' && ci == ei) {
                        // If we start the object, reset array indices
                        ed++, pos++, ci = ei = -1;
                    }
                    nesting[depth++] = c;
                    expecting = S_KEY;
                    break;
                } else if (c == '[') {
                    if (depth >= (int)sizeof(nesting)) return -3;
                    if (depth == ed && path[pos] == '[' && ei == ci) {
                        ed++, pos++, ci = 0;
                        for (ei = 0; path[pos] != ']' && path[pos] != '\0'; pos++) {
                            ei *= 10;
                            ei += path[pos] - '0';
                        }
                        if (path[pos] != 0) pos++;
                    }
                    nesting[depth++] = c;
                    break;
                } else if (c == ']' && depth > 0) {  // Empty array
                    MG_EOO();
                } else if (c == 't' && i + 3 < len && memcmp(&s[i], "true", 4) == 0) {
                    i += 3;
                } else if (c == 'n' && i + 3 < len && memcmp(&s[i], "null", 4) == 0) {
                    i += 3;
                } else if (c == 'f' && i + 4 < len && memcmp(&s[i], "false", 5) == 0) {
                    i += 4;
                } else if (c == '-' || ((c >= '0' && c <= '9'))) {
                    char k = s[i];
                    while (k == '-' || k == '.' || k == 'x' || k == 'e' || k == 'E' || (k >= '0' && k <= '9')) {
                        i++;
                        k = s[i];
                    }
                    i--;
                } else if (c == '"') {
                    int n = json_pass_string(&s[i + 1], len - i - 1);
                    if (n < 0) return n;
                    i += n + 1;
                } else {
                    return -1;
                }
                MG_CHECKRET();
                if (depth == ed && ei >= 0) ci++;
                expecting = S_COMMA_OR_EOO;
                break;

            case S_KEY:
                if (c == '"') {
                    int n = json_pass_string(&s[i + 1], len - i - 1);
                    if (n < 0) return n;
                    if (i + 1 + n >= len) return -2;
                    if (depth < ed) return -2;
                    if (depth == ed && path[pos - 1] != '.') return -2;
                    if (depth == ed && path[pos - 1] == '.' && strncmp(&s[i + 1], &path[pos], (size_t)n) == 0 &&
                        (path[pos + n] == '\0' || path[pos + n] == '.' || path[pos + n] == '[')) {
                        pos += n;
                    }
                    i += n + 1;
                    expecting = S_COLON;
                } else if (c == '}') {  // Empty object
                    MG_EOO();
                    expecting = S_COMMA_OR_EOO;
                    if (depth == ed && ei >= 0) ci++;
                } else {
                    return -1;
                }
                break;

            case S_COLON:
                if (c == ':') {
                    expecting = S_VALUE;
                } else {
                    return -1;
                }
                break;

            case S_COMMA_OR_EOO:
                if (depth <= 0) {
                    return -1;
                } else if (c == ',') {
                    expecting = (nesting[depth - 1] == '{') ? S_KEY : S_VALUE;
                } else if (c == ']' || c == '}') {
                    MG_EOO();
                    if (depth == ed && ei >= 0) ci++;
                } else {
                    return -1;
                }
                break;
        }
    }
    return -2;
}

static unsigned char xnimble(unsigned char c) {
    return (c >= '0' && c <= '9')   ? (unsigned char)(c - '0')
           : (c >= 'A' && c <= 'F') ? (unsigned char)(c - '7')
                                    : (unsigned char)(c - 'W');
}

static unsigned long xunhexn(const char *s, size_t len) {
    unsigned long i = 0, v = 0;
    for (i = 0; i < len; i++) v <<= 4, v |= xnimble(((unsigned char *)s)[i]);
    return v;
}

static int json_unescape(const char *buf, size_t len, char *to, size_t n) {
    size_t i, j;
    for (i = 0, j = 0; i < len && j < n; i++, j++) {
        if (buf[i] == '\\' && i + 5 < len && buf[i + 1] == 'u') {
            //  \uXXXX escape. We could process a simple one-byte chars
            // \u00xx from the ASCII range. More complex chars would require
            // dragging in a UTF8 library, which is too much for us
            if (buf[i + 2] != '0' || buf[i + 3] != '0') return -1;  // Give up
            ((unsigned char *)to)[j] = (unsigned char)xunhexn(buf + i + 4, 2);
            i += 5;
        } else if (buf[i] == '\\' && i + 1 < len) {
            char c = json_esc(buf[i + 1], 0);
            if (c == 0) return -1;
            to[j] = c;
            i++;
        } else {
            to[j] = buf[i];
        }
    }
    if (j >= n) return -1;
    if (n > 0) to[j] = '\0';
    return (int)j;
}

int json_get_num(const char *buf, int len, const char *path, long *v) {
    int found = 0, n = 0, off = json_get(buf, len, path, &n);
    char **pptr;
    char *ptr;
    char int_buf[20] = {0};
    if (off >= 0 && (buf[off] == '-' || (buf[off] >= '0' && buf[off] <= '9'))) {
        if (v != NULL) {
            // *v = xatod(buf + off, n, NULL);
            for(int i = 0; i < off + 20 && buf[i + off] != '\0' && buf[i + off] != ',' && buf[i + off] != ']'; i++){
                int_buf[i] = buf[i + off];
            }
            ptr =(char*) int_buf;
            pptr = &ptr;
            xatoi(pptr, v);
        }
        found = 1;
    }
    return found;
}

/*
make sure that you have enough buffer space for expansion
*/
int json_set_num(char *buf, int len, const char *path, long val){
    char val_buf[10] = {0};
    int new_val_len = 0;
    int old_val_len = 0;
    xsprintf(val_buf, "%d", val);
    new_val_len = strlen(val_buf);

    int offset = json_get(buf, len, path, &old_val_len);
    if(offset > 0){
        if(new_val_len == old_val_len){
            memcpy(buf + offset, val_buf, old_val_len);
        } else {
            if(old_val_len < new_val_len){
                for(int i = len; i >= offset + old_val_len; i--){
                    buf[i + (new_val_len - old_val_len)] = buf[i];
                }
            }else {
                for(int i = offset + new_val_len; i < len - (old_val_len - new_val_len); i++){
                    buf[i] = buf[i + (old_val_len - new_val_len)];
                }
            }
            memcpy(buf + offset, val_buf, new_val_len);
        }
    }
    return offset < 0 ? 1 : 0;
}

int json_get_bool(const char *buf, int len, const char *path, int *v) {
    int found = 0, off = json_get(buf, len, path, NULL);
    if (off >= 0 && (buf[off] == 't' || buf[off] == 'f')) {
        if (v != NULL) *v = buf[off] == 't';
        found = 1;
    }
    return found;
}

int json_get_str(const char *buf, int len, const char *path, char *dst, size_t dlen) {
    int result = -1, n = 0, off = json_get(buf, len, path, &n);
    if (off >= 0 && n > 1 && buf[off] == '"') {
        result = json_unescape(buf + off + 1, (size_t)(n - 2), dst, dlen);
    }
    return result;
}
