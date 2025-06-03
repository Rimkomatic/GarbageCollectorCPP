#pragma once
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cstdint>
#include <unistd.h>
#include <fstream>

struct Header {
    unsigned int size;
    Header* next;
};

static Header base;
static Header* freep;
static Header* usedp;
static uintptr_t stack_bottom;

static void add_to_free_list(Header* bp) {
    Header* p;
    for (p = freep; !(bp > p && bp < p->next); p = p->next)
        if (p >= p->next && (bp > p || bp < p->next))
            break;

    if (bp + bp->size == p->next) {
        bp->size += p->next->size;
        bp->next = p->next->next;
    } else
        bp->next = p->next;

    if (p + p->size == bp) {
        p->size += bp->size;
        p->next = bp->next;
    } else
        p->next = bp;

    freep = p;
}

#define MIN_ALLOC_SIZE 4096

static Header* morecore(size_t num_units) {
    void* vp;
    Header* up;

    if (num_units < MIN_ALLOC_SIZE / sizeof(Header))
        num_units = MIN_ALLOC_SIZE / sizeof(Header);

    vp = sbrk(num_units * sizeof(Header));
    if (vp == (void*)-1)
        return nullptr;

    up = (Header*)vp;
    up->size = num_units;
    add_to_free_list(up);
    return freep;
}

void* GC_malloc(size_t alloc_size) {
    size_t num_units;
    Header* p;
    Header* prevp;

    num_units = (alloc_size + sizeof(Header) - 1) / sizeof(Header) + 1;

    if ((prevp = freep) == nullptr) {
        base.next = freep = prevp = &base;
        base.size = 0;
    }

    for (p = prevp->next;; prevp = p, p = p->next) {
        if (p->size >= num_units) {
            if (p->size == num_units)
                prevp->next = p->next;
            else {
                p->size -= num_units;
                p += p->size;
                p->size = num_units;
            }

            freep = prevp;

            if (!usedp)
                usedp = p->next = p;
            else {
                p->next = usedp->next;
                usedp->next = p;
            }

            return (void*)(p + 1);
        }
        if (p == freep) {
            p = morecore(num_units);
            if (!p)
                return nullptr;
        }
    }
}

#define UNTAG(p) ((Header*)((uintptr_t)(p) & ~0x3))

static void scan_region(uintptr_t* sp, uintptr_t* end) {
    Header* bp;
    for (; sp < end; ++sp) {
        uintptr_t v = *sp;
        bp = usedp;
        do {
            if ((uintptr_t)(bp + 1) <= v && (uintptr_t)(bp + 1 + bp->size) > v) {
                bp->next = (Header*)((uintptr_t)(bp->next) | 1);
                break;
            }
        } while ((bp = UNTAG(bp->next)) != usedp);
    }
}

static void scan_heap() {
    uintptr_t* vp;
    Header* bp;
    Header* up;

    for (bp = UNTAG(usedp->next); bp != usedp; bp = UNTAG(bp->next)) {
        if (!((uintptr_t)bp->next & 1)) continue;

        for (vp = (uintptr_t*)(bp + 1); vp < (uintptr_t*)(bp + 1 + bp->size); ++vp) {
            uintptr_t v = *vp;
            up = UNTAG(usedp->next);
            do {
                if (up != bp && (uintptr_t)(up + 1) <= v && (uintptr_t)(up + 1 + up->size) > v) {
                    up->next = (Header*)((uintptr_t)up->next | 1);
                    break;
                }
            } while ((up = UNTAG(up->next)) != usedp);
        }
    }
}

void GC_init() {
    static bool initted = false;
    if (initted) return;
    initted = true;

    std::ifstream statfp("/proc/self/stat");
    assert(statfp && "Cannot open /proc/self/stat");

    std::string discard;
    for (int i = 0; i < 27; ++i) statfp >> discard;
    statfp >> stack_bottom;

    usedp = nullptr;
    base.next = freep = &base;
    base.size = 0;
}

void GC_collect() {
    if (!usedp) return;

    extern char etext, end;
    scan_region((uintptr_t*)&etext, (uintptr_t*)&end);

    uintptr_t stack_top;
    asm volatile("mov %%rbp, %0" : "=r"(stack_top));
    scan_region((uintptr_t*)stack_top, (uintptr_t*)stack_bottom);

    scan_heap();

    Header* p;
    Header* prevp;
    Header* tp;

    for (prevp = usedp, p = UNTAG(usedp->next);; prevp = p, p = UNTAG(p->next)) {
    next_chunk:
        if (!((uintptr_t)p->next & 1)) {
            tp = p;
            p = UNTAG(p->next);
            add_to_free_list(tp);
            if (usedp == tp) {
                usedp = nullptr;
                break;
            }
            prevp->next = (Header*)((uintptr_t)p | ((uintptr_t)prevp->next & 1));
            goto next_chunk;
        }
        p->next = (Header*)((uintptr_t)p->next & ~1);
        if (p == usedp) break;
    }
}
