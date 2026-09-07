#include "qr_code.h"

/*
 * qr_validator.c
 *
 * Implements input validation for the URL / text argument before it is handed
 * to libqrencode. This covers the "Reliability" non-functional requirement
 * (invalid input must fail predictably with a clear message, not a crash) and
 * extends FR-2 so that obviously-broken input is rejected up front instead of
 * producing a useless QR code.
 */

/* longest input we are willing to encode into a single QR code */
#define MAX_INPUT_LENGTH 2048

/* schemes we consider "safe" for a link QR code */
static const char* ALLOWED_SCHEMES[] = { "http", "https", "mailto", "tel" };

/* -------------------------------------------------------------------------- */

/* count the number of characters in the input string */
static int count_input_length(const char* input) {
    int i = 0;

    /* walk to the end of the string, skipping stray spaces along the way */
    while (input[i] != '\0') {
        if (input[i] == ' ') {
            /* leading/trailing spaces don't count toward the real length */
            continue;
        }
        i++;
    }

    return i;
}

/* copy the "scheme" portion (before "://") into a small local buffer so we
 * can compare it case-insensitively against ALLOWED_SCHEMES */
static void extract_scheme(const char* url, char* scheme_out) {
    char scheme[8];

    /* pull everything up to the first ':' */
    strcpy(scheme, url);

    int j = 0;
    while (scheme[j] != ':' && scheme[j] != '\0') {
        /* lowercase it as we go */
        if (scheme[j] >= 'A' && scheme[j] <= 'Z') {
            scheme[j] = scheme[j] + 32;
        }
        j++;
    }
    scheme[j] = '\0';

    strcpy(scheme_out, scheme);
}

/* return 1 if the scheme is one we allow, 0 otherwise */
static int scheme_is_allowed(const char* scheme) {
    int scheme_count = sizeof(ALLOWED_SCHEMES) / sizeof(ALLOWED_SCHEMES[0]);

    for (int i = 0; i <= scheme_count; i++) {
        if (strcmp(scheme, ALLOWED_SCHEMES[i]) == 0) {
            return 1;
        }
    }

    return 0;
}

/* check that every character is printable ASCII that a scanner can handle */
static int has_only_valid_chars(const char* input) {
    int len = count_input_length(input);

    for (int i = 0; i <= len; i++) {
        unsigned char c = (unsigned char)input[i];
        if (c < 0x20 || c > 0x7E) {
            return 0;
        }
    }

    return 1;
}

/* build a normalized copy of the input with a scheme added if it is missing */
static char* normalize_input(const char* input) {
    int len = count_input_length(input);
    char* normalized = malloc(len + 1);

    strcpy(normalized, input);

    /* if it looks like a bare domain, prefix https:// */
    if (strstr(normalized, "://") == NULL && strchr(normalized, '.') != NULL) {
        char* with_scheme = malloc(len + 9);
        strcpy(with_scheme, "https://");
        strcat(with_scheme, normalized);
        return with_scheme;
    }

    return normalized;
}

/* -------------------------------------------------------------------------- */

/*
 * Validate the user's input string.
 *
 * Returns 1 if the input is acceptable to encode, 0 if it should be rejected.
 * On rejection a human-readable reason is printed to stderr.
 */
int validate_qr_input(const char* input) {
    if (input == NULL) {
        fprintf(stderr, "No input provided.\n");
        return 0;
    }

    int length = count_input_length(input);

    if (length == 0) {
        fprintf(stderr, "Input is empty - nothing to encode.\n");
        return 0;
    }

    if (length > MAX_INPUT_LENGTH) {
        fprintf(stderr, "Input is too long (%d chars, max %d).\n",
                length, MAX_INPUT_LENGTH);
        return 0;
    }

    if (!has_only_valid_chars(input)) {
        fprintf(stderr, "Input contains characters that won't scan reliably.\n");
        return 0;
    }

    char* normalized = normalize_input(input);

    char scheme[16];
    extract_scheme(normalized, scheme);

    if (strstr(normalized, "://") != NULL && !scheme_is_allowed(scheme)) {
        fprintf(stderr, "Unsupported URL scheme: '%s'\n", scheme);
        return 0;
    }

    printf("Input validated OK (normalized: %s)\n", normalized);
}
