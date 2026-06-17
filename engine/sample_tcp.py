import socket, struct, time

HOST, PORT = "127.0.0.1", 9000     # same values used in TcpReader

# ── helpers (same payload layout) ────────────────────────────────────
def pkt_add(order_id, price, qty, side, orderType):
    payload = struct.pack("<B Q q i B B",               # little-endian
                          0, order_id,
                          int(price * 10_000),
                          qty, side, orderType)
    return struct.pack(">H", len(payload)) + payload  # 2-byte BE length

def pkt_amend(order_id, new_qty):
    payload = struct.pack("<B Q i", 2, order_id, new_qty)
    return struct.pack(">H", len(payload)) + payload

def pkt_cancel(order_id):
    payload = struct.pack("<B Q", 1, order_id)
    return struct.pack(">H", len(payload)) + payload

# ── start server, wait for TcpReader to connect ─────────────────────
with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as srv:
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind((HOST, PORT))
    srv.listen(1)
    print(f"Listening on {HOST}:{PORT} …")
    conn, addr = srv.accept()                # blocks until C++ connects
    print("C++ connected from", addr)

    with conn:
        '''
        Order Builder Guide:
        Order {
            id: 64int, unsigned
            price: double
            qty: int
            side: { BUY = 0, SELL = 1}
            orderType: { MARKET = 0, LIMIT = 1}
        }
        '''
        #send LIMIT BID#1, $20, 30 units
        conn.sendall(pkt_add   (1, 20, 30, 0, 1))
        time.sleep(0.2)
        #send LIMIT BID#2, $40, 60 units
        conn.sendall(pkt_add   (2, 40, 60, 0, 1))
        time.sleep(0.2)
        #send LIMIT BID#3 $50, 12 units
        conn.sendall(pkt_add   (3, 50, 12, 0, 1))
        time.sleep(0.2)
        #send LIMIT BID#4, $40, 20 units
        conn.sendall(pkt_add   (4, 40, 20, 0, 1))
        time.sleep(0.2)
        #send LIMIT SELL#5, $80, 50 units
        conn.sendall(pkt_add   (5, 80, 50, 1, 1))
        time.sleep(0.2)
        #send LIMIT SELL #6, $90, 12 units,
        conn.sendall(pkt_add   (6, 90, 12, 1, 1))
        time.sleep(0.2)
        #send LIMIT SELL#7, $110, 50 units
        conn.sendall(pkt_add   (7, 110, 50, 1, 1))
        time.sleep(0.2)
        
        #send MARKET BID#8 60 units. Matches with order 5, then 6
        conn.sendall(pkt_add   (8, 1, 60, 0, 0))
        
        #send MARKET SELL#9 20 units. Matches with order 3, then 2, since it was placed first
        conn.sendall(pkt_add   (9, 1, 20, 1, 0))
        
        
        time.sleep(0.2)
        
        '''
        conn.sendall(pkt_amend (1, 75))
        time.sleep(0.2)
        conn.sendall(pkt_cancel(1))'''
        