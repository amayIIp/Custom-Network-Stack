import struct
import sys

def verify_itch(file_path):
    counts = {
        'total': 0, 'S': 0, 'A': 0, 'E': 0, 'X': 0, 'D': 0, 'P': 0, 'other': 0
    }
    shares = {
        'A': 0, 'E': 0, 'X': 0, 'P': 0
    }
    min_price = 0xFFFFFFFF
    max_price = 0
    
    with open(file_path, 'rb') as f:
        data = f.read()
    
    offset = 0
    length = len(data)
    
    while offset + 2 <= length:
        msg_len, = struct.unpack('>H', data[offset:offset+2])
        if offset + 2 + msg_len > length:
            print("Truncated message at offset", offset)
            break
            
        msg = data[offset+2:offset+2+msg_len]
        msg_type = chr(msg[0])
        
        counts['total'] += 1
        if msg_type in counts:
            counts[msg_type] += 1
        else:
            counts['other'] += 1
            
        if msg_type == 'A':
            # Add Order Msg (36 bytes): type(1), locate(2), track(2), ts(6), ref(8), side(1), shares(4), stock(8), price(4)
            _, _, _, _, _, _, sh, _, pr = struct.unpack('>BHH6sQcI8sI', msg)
            shares['A'] += sh
            if pr > 0:
                if pr < min_price:
                    min_price = pr
                if pr > max_price:
                    max_price = pr
        elif msg_type == 'E':
            # Order Executed (30 bytes): type(1), locate(2), track(2), ts(6), ref(8), sh(4), match(8)
            _, _, _, _, _, sh, _ = struct.unpack('>BHH6sQIQ', msg)
            shares['E'] += sh
        elif msg_type == 'X':
            # Order Cancel (22 bytes): type(1), locate(2), track(2), ts(6), ref(8), sh(4)
            _, _, _, _, _, sh = struct.unpack('>BHH6sQI', msg)
            shares['X'] += sh
        elif msg_type == 'P':
            # Trade Message (44 bytes): type(1), locate(2), track(2), ts(6), ref(8), side(1), sh(4), stock(8), price(4), match(8)
            _, _, _, _, _, _, sh, _, pr, _ = struct.unpack('>BHH6sQcI8sIQ', msg)
            shares['P'] += sh
            if pr > 0:
                if pr < min_price:
                    min_price = pr
                if pr > max_price:
                    max_price = pr
            
        offset += 2 + msg_len
        
    print(f"Stats for {file_path}:")
    print(f"  Total Messages: {counts['total']}")
    for k, v in counts.items():
        if k != 'total':
            print(f"  Count {k}: {v}")
    for k, v in shares.items():
        print(f"  Shares {k}: {v}")
    print(f"  Min Price: {min_price}")
    print(f"  Max Price: {max_price}")

if __name__ == '__main__':
    path = sys.argv[1] if len(sys.argv) > 1 else 'itch-parser/data/01302019.NASDAQ_ITCH50.25MB.bin'
    verify_itch(path)
