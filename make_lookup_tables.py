#!/usr/bin/env python

def make_lookup_tables() -> list[list[int]]:
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
