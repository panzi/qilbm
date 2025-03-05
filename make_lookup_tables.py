#!/usr/bin/env python

def make_lookup_tables_old() -> list[list[int]]:
    tables: list[list[int]] = [[]]
    for bits in range(1, 8):
        max_short_val = 0xFF >> (8 - bits)
        short_val_count = max_short_val + 1
        table: list[int] = []
        for short_val in range(0, short_val_count):
            fshort_val = short_val / max_short_val
            min_diff = 1.0
            best_color = 0
            for long_val in range(0, 256):
                flong_val = long_val / 255
                diff = abs(flong_val - fshort_val)
                if min_diff > diff:
                    min_diff = diff
                    best_color = long_val
            table.append(best_color)
        tables.append(table)

    return tables

# see: https://threadlocalmutex.com/?page_id=60
conv_funcs = [
    lambda x: 0,
    lambda x: x * 255,
    lambda x: x * 85,
    lambda x: (x * 146 + 1) >> 2,
    lambda x: x * 17,
    lambda x: (x * 527 + 23) >> 6,
    lambda x: (x * 259 + 33) >> 6,
    lambda x: (x * 257 + 64) >> 7,
]

def make_lookup_tables() -> list[list[int]]:
    tables: list[list[int]] = [[]]
    for bits in range(1, 8):
        conv = conv_funcs[bits]
        max_short_val = 0xFF >> (8 - bits)
        short_val_count = max_short_val + 1
        table: list[int] = []
        for short_val in range(0, short_val_count):
            table.append(conv(short_val))
        tables.append(table)

    return tables

def print_lookup_tables(tables: list[list[int]]) -> None:
    for bits, table in enumerate(tables):
        if bits == 1:
            print(f'static const uint8_t COLOR_LOOKUP_TABLE_{bits}BIT[] = {{{",".join(f' {val}' for val in table)} }};')
        elif bits > 1:
            print(f'static const uint8_t COLOR_LOOKUP_TABLE_{bits}BITS[] = {{{",".join(f' {val}' for val in table)} }};')
    print()
    print(f"static const uint8_t *COLOR_LOOKUP_TABLES[{len(tables)}] = {{")
    print(f'    NULL,')
    print(f'    COLOR_LOOKUP_TABLE_1BIT,')
    for bits in range(2, len(tables)):
        print(f'    COLOR_LOOKUP_TABLE_{bits}BITS,')
    print('};')

if __name__ == '__main__':
    print_lookup_tables(make_lookup_tables())
