#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL, strtod() */
#include <errno.h>
#include <stdio.h>
#include <math.h>    /* HUGE_VAL */

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')

typedef struct {
    const char* json;
}lept_context;

static void lept_parse_whitespace(lept_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}

static int lept_parse_literal(lept_context* c, lept_value* v, const char* literal, lept_type type) {
	EXPECT(c, literal[0]);
    size_t i;
    for (i = 1; literal[i]; i++) {
        if (c->json[i - 1] != literal[i])
            return LEPT_PARSE_INVALID_VALUE;
    }
    c->json += i - 1;
    v->type = type;
	return LEPT_PARSE_OK;
}

static int lept_parse_number(lept_context* c, lept_value* v) {
    char* end;
    /* \TODO validate number */
	const char* p = c->json;
    /* optional minus*/
    if (*p == '-')
		p++;

    /* integer part */
    if (*p == '0') {
        /* single zero is valid; must not be followed by another digit
           If it is followed by a digit or other invalid token (like 'x'),
           treat it as trailing root-not-singular (so caller will report that). */
        p++;
        if (ISDIGIT(*p))
            return LEPT_PARSE_ROOT_NOT_SINGULAR;
        /* allow '.', 'e', 'E', whitespace or '\0' to follow; other chars (like 'x') should be reported
           as root-not-singular so top-level parser can detect trailing tokens. */
        if (*p != '.' && *p != 'e' && *p != 'E' && *p != '\0' && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r')
            return LEPT_PARSE_ROOT_NOT_SINGULAR;
    }
    else {
        if (!ISDIGIT1TO9(*p)) return LEPT_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++);
    }
	/* fraction part */
    if (*p == '.') {
		p++;
		if (!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE; /* at least one digit after '.' */
		while (ISDIGIT(*p)) p++;
    }

	/* exponent part */
    if (*p == 'e' || *p == 'E') {
        p++;
        if (*p == '+' || *p == '-') p++;
		if (!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE; /* at least one digit in exponent */
        while (ISDIGIT(*p)) p++;
    }

	errno = 0;
    v->n = strtod(c->json, &end);
    if (v -> n == HUGE_VAL || v -> n == -HUGE_VAL) return LEPT_PARSE_NUMBER_TOO_BIG;
    if (c->json == end)
        return LEPT_PARSE_INVALID_VALUE;
    c->json = end;
    v->type = LEPT_NUMBER;
    return LEPT_PARSE_OK;
}

static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
        case 't':  return lept_parse_literal(c, v, "true", LEPT_TRUE);
        case 'f':  return lept_parse_literal(c, v, "false", LEPT_FALSE);
        case 'n':  return lept_parse_literal(c, v, "null", LEPT_NULL);
        default:   return lept_parse_number(c, v);
        case '\0': return LEPT_PARSE_EXPECT_VALUE;
    }
}

int lept_parse(lept_value* v, const char* json) {
    lept_context c;
    int ret;
    assert(v != NULL);
    c.json = json;
    v->type = LEPT_NULL;
    lept_parse_whitespace(&c);
    if ((ret = lept_parse_value(&c, v)) == LEPT_PARSE_OK) {
        lept_parse_whitespace(&c);
        if (*c.json != '\0') {
            v->type = LEPT_NULL;
            ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    return ret;
}

lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}

double lept_get_number(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->n;
}
