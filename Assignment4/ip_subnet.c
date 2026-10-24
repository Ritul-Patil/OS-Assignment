/*
 * IP Address Allocation and Subnet Identification (IPv4)
 *
 * Features:
 *   1. Validate an IP address and show its subnet details
 *      (class, network address, broadcast address, usable host range, hosts)
 *   2. Check whether two hosts belong to the same subnet
 *   3. Simulate IP address allocation to hosts from a subnet
 *
 * Subnet masks can be entered in CIDR form (/26 or 26) or dotted form
 * (255.255.255.192). IP addresses must be in dotted decimal form with four
 * octets in the range 0-255 (leading zeros such as "01" are rejected).
 *
 * Compile: gcc -Wall -Wextra -o ipsubnet ip_subnet.c
 * Run:     ./ipsubnet
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#define LINE_LEN 128

/* ---------- parsing and validation ---------- */

/* Remove trailing newline / spaces and skip leading spaces. */
static char *trim(char *s) {
    while (isspace((unsigned char)*s))
        s++;
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1]))
        s[--len] = '\0';
    return s;
}

/* Strictly parse a dotted-decimal IPv4 address. Returns 1 if valid. */
static int parse_ip(const char *text, uint32_t *out) {
    uint32_t value = 0;
    int octets = 0;
    const char *p = text;

    while (1) {
        int digits = 0, num = 0;
        const char *start = p;

        while (isdigit((unsigned char)*p)) {
            num = num * 10 + (*p - '0');
            digits++;
            p++;
            if (digits > 3)
                return 0;
        }
        if (digits == 0)
            return 0;                       /* empty octet or bad char */
        if (digits > 1 && *start == '0')
            return 0;                       /* leading zero */
        if (num > 255)
            return 0;

        value = (value << 8) | (uint32_t)num;
        octets++;

        if (*p == '.') {
            p++;
            if (octets == 4)
                return 0;                   /* more than four octets */
        } else if (*p == '\0') {
            break;
        } else {
            return 0;                       /* unexpected character */
        }
    }

    if (octets != 4)
        return 0;
    *out = value;
    return 1;
}

static uint32_t mask_from_cidr(int cidr) {
    return cidr == 0 ? 0u : (0xFFFFFFFFu << (32 - cidr));
}

static int cidr_from_mask(uint32_t mask) {
    int bits = 0;
    while (mask & 0x80000000u) {
        bits++;
        mask <<= 1;
    }
    return bits;
}

/* A valid mask is a run of 1s followed by a run of 0s. */
static int mask_is_contiguous(uint32_t mask) {
    uint32_t inverted = ~mask;
    return (inverted & (inverted + 1)) == 0;
}

/* Accepts "/26", "26" or "255.255.255.192". Returns 1 if valid. */
static int parse_mask(const char *text, uint32_t *out) {
    if (strchr(text, '.') != NULL) {
        uint32_t m;
        if (!parse_ip(text, &m) || !mask_is_contiguous(m))
            return 0;
        *out = m;
        return 1;
    }

    const char *p = (text[0] == '/') ? text + 1 : text;
    if (*p == '\0')
        return 0;
    int cidr = 0, digits = 0;
    while (isdigit((unsigned char)*p)) {
        cidr = cidr * 10 + (*p - '0');
        digits++;
        p++;
        if (digits > 2)
            return 0;
    }
    if (*p != '\0' || cidr > 32)
        return 0;
    *out = mask_from_cidr(cidr);
    return 1;
}

/* ---------- helpers ---------- */

static void print_ip(uint32_t ip) {
    printf("%u.%u.%u.%u", (ip >> 24) & 255, (ip >> 16) & 255,
           (ip >> 8) & 255, ip & 255);
}

static char ip_class(uint32_t ip) {
    uint32_t first = ip >> 24;
    if (first < 128) return 'A';
    if (first < 192) return 'B';
    if (first < 224) return 'C';
    if (first < 240) return 'D';
    return 'E';
}

static const char *ip_kind(uint32_t ip) {
    uint32_t a = ip >> 24, b = (ip >> 16) & 255;
    if (a == 10) return "Private";
    if (a == 172 && b >= 16 && b <= 31) return "Private";
    if (a == 192 && b == 168) return "Private";
    if (a == 127) return "Loopback";
    if (a == 169 && b == 254) return "Link-local (APIPA)";
    return "Public";
}

static void read_line(const char *prompt, char *buf, size_t size) {
    printf("%s", prompt);
    fflush(stdout);
    if (fgets(buf, (int)size, stdin) == NULL) {
        printf("\nInput ended.\n");
        exit(0);
    }
}

/* Ask for an IP until we get one (or report once and return 0). */
static int ask_ip(const char *prompt, uint32_t *ip) {
    char buf[LINE_LEN];
    read_line(prompt, buf, sizeof buf);
    char *s = trim(buf);
    if (!parse_ip(s, ip)) {
        printf("  -> INVALID IP address: \"%s\"\n", s);
        return 0;
    }
    printf("  -> Valid IP address\n");
    return 1;
}

static int ask_mask(const char *prompt, uint32_t *mask) {
    char buf[LINE_LEN];
    read_line(prompt, buf, sizeof buf);
    char *s = trim(buf);
    if (!parse_mask(s, mask)) {
        printf("  -> INVALID subnet mask: \"%s\"\n", s);
        return 0;
    }
    printf("  -> Valid subnet mask (/%d)\n", cidr_from_mask(*mask));
    return 1;
}

/* ---------- features ---------- */

static void subnet_info(void) {
    uint32_t ip, mask;
    if (!ask_ip("Enter IP address: ", &ip))
        return;
    if (!ask_mask("Enter subnet mask (CIDR like /26 or dotted): ", &mask))
        return;

    uint32_t network = ip & mask;
    uint32_t broadcast = network | ~mask;
    int cidr = cidr_from_mask(mask);

    printf("\n--- Subnet details ---\n");
    printf("IP address     : "); print_ip(ip); printf("\n");
    printf("Subnet mask    : "); print_ip(mask); printf(" (/%d)\n", cidr);
    printf("Class          : %c\n", ip_class(ip));
    printf("Type           : %s\n", ip_kind(ip));
    printf("Network address: "); print_ip(network); printf("\n");

    if (cidr >= 31) {
        printf("Broadcast      : %s\n", cidr == 31 ? "not used (point-to-point /31)" : "n/a (single host /32)");
        printf("Usable hosts   : %d\n", cidr == 31 ? 2 : 1);
        printf("Host range     : "); print_ip(network);
        if (cidr == 31) { printf(" - "); print_ip(broadcast); }
        printf("\n");
    } else {
        uint32_t total = 1u << (32 - cidr);
        printf("Broadcast      : "); print_ip(broadcast); printf("\n");
        printf("Total addresses: %u\n", total);
        printf("Usable hosts   : %u\n", total - 2);
        printf("Host range     : "); print_ip(network + 1);
        printf(" - "); print_ip(broadcast - 1); printf("\n");
    }

    if (ip == network && cidr < 31)
        printf("Note: this address is the network address (cannot be assigned to a host).\n");
    if (ip == broadcast && cidr < 31)
        printf("Note: this address is the broadcast address (cannot be assigned to a host).\n");
}

static void same_subnet(void) {
    uint32_t ip1, ip2, mask;
    if (!ask_ip("Enter IP address of Host 1: ", &ip1))
        return;
    if (!ask_ip("Enter IP address of Host 2: ", &ip2))
        return;
    if (!ask_mask("Enter subnet mask (CIDR like /26 or dotted): ", &mask))
        return;

    uint32_t net1 = ip1 & mask, net2 = ip2 & mask;

    printf("\n--- Same subnet check ---\n");
    printf("Host 1 network : "); print_ip(net1); printf("\n");
    printf("Host 2 network : "); print_ip(net2); printf("\n");
    if (net1 == net2)
        printf("Result: Host 1 and Host 2 are in the SAME subnet.\n");
    else
        printf("Result: Host 1 and Host 2 are in DIFFERENT subnets.\n");
}

static void allocate_ips(void) {
    uint32_t ip, mask;
    char buf[LINE_LEN];

    if (!ask_ip("Enter any IP address in the subnet: ", &ip))
        return;
    if (!ask_mask("Enter subnet mask (CIDR like /26 or dotted): ", &mask))
        return;

    int cidr = cidr_from_mask(mask);
    if (cidr > 30) {
        printf("Subnet too small for allocation simulation (use /30 or larger).\n");
        return;
    }

    read_line("Enter number of hosts needing an IP address: ", buf, sizeof buf);
    char *s = trim(buf);
    char *end;
    long want = strtol(s, &end, 10);
    if (*s == '\0' || *end != '\0' || want < 1 || want > 100000) {
        printf("  -> INVALID number of hosts: \"%s\"\n", s);
        return;
    }

    uint32_t network = ip & mask;
    uint32_t broadcast = network | ~mask;
    uint32_t usable = (1u << (32 - cidr)) - 2;
    uint32_t allocated = (uint32_t)want < usable ? (uint32_t)want : usable;

    printf("\n--- IP allocation simulation ---\n");
    printf("Subnet : "); print_ip(network); printf("/%d\n", cidr);
    printf("Usable addresses: %u, requested: %ld\n\n", usable, want);

    printf("%-8s %-18s %s\n", "Host", "IP address", "Status");
    printf("-----------------------------------------\n");

    const uint32_t show_limit = 20;         /* keep output readable */
    for (uint32_t i = 0; i < (uint32_t)want; i++) {
        if (i >= show_limit && i < (uint32_t)want - 1) {
            if (i == show_limit)
                printf("...      (%ld more rows hidden)\n", want - show_limit - 1);
            continue;
        }
        char name[16];
        snprintf(name, sizeof name, "Host%u", i + 1);
        printf("%-8s ", name);
        if (i < usable) {
            char addr[20];
            uint32_t a = network + 1 + i;
            snprintf(addr, sizeof addr, "%u.%u.%u.%u", (a >> 24) & 255,
                     (a >> 16) & 255, (a >> 8) & 255, a & 255);
            printf("%-18s Allocated\n", addr);
        } else {
            printf("%-18s Not allocated (subnet full)\n", "-");
        }
    }

    printf("\nAllocated: %u of %ld hosts\n", allocated, want);
    printf("Remaining free addresses: %u\n", usable - allocated);
    if ((uint32_t)want > usable)
        printf("%u host(s) could not be allocated. Use a larger subnet (smaller CIDR value).\n",
               (uint32_t)want - usable);
    printf("Reserved: network "); print_ip(network);
    printf(", broadcast "); print_ip(broadcast); printf("\n");
}

int main(void) {
    char buf[LINE_LEN];

    while (1) {
        printf("\n===== IP Address & Subnet Tool =====\n");
        printf("1. Validate IP and identify subnet\n");
        printf("2. Check if two hosts are in the same subnet\n");
        printf("3. Simulate IP address allocation\n");
        printf("4. Exit\n");
        read_line("Enter choice: ", buf, sizeof buf);

        char *s = trim(buf);
        if (strcmp(s, "1") == 0)
            subnet_info();
        else if (strcmp(s, "2") == 0)
            same_subnet();
        else if (strcmp(s, "3") == 0)
            allocate_ips();
        else if (strcmp(s, "4") == 0) {
            printf("Goodbye.\n");
            break;
        } else
            printf("Invalid choice, enter 1-4.\n");
    }
    return 0;
}