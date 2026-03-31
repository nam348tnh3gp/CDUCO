#!/usr/bin/env python3
"""
Duino-Coin Miner - iOS Compatible (Threading version)
No multiprocessing - works on a-Shell and iSH
"""

from time import time, sleep
from hashlib import sha1
from socket import socket
import threading
from threading import Thread, Lock
from datetime import datetime
from random import randint
from queue import Queue
import sys
import os
import json
import urllib.parse
from pathlib import Path
from re import sub
from random import choice
from signal import signal, SIGINT
from configparser import ConfigParser

debug = False
configparser = ConfigParser()
printlock = Lock()
print_queue = Queue()

# Install requests if needed
try:
    import requests
except:
    import subprocess
    subprocess.check_call([sys.executable, '-m', 'pip', 'install', 'requests'])
    import requests

# Install colorama if needed
try:
    from colorama import Back, Fore, Style, init
    init(autoreset=True)
except:
    import subprocess
    subprocess.check_call([sys.executable, '-m', 'pip', 'install', 'colorama'])
    from colorama import Back, Fore, Style, init
    init(autoreset=True)


class Settings:
    ENCODING = "UTF8"
    SEPARATOR = ","
    VER = 4.3
    DATA_DIR = "Duino-Coin_Miner"
    TRANSLATIONS = "https://raw.githubusercontent.com/revoxhere/duino-coin/master/Resources/PC_Miner_langs.json"
    TRANSLATIONS_FILE = "/Translations.json"
    SETTINGS_FILE = "/Settings.cfg"
    SOC_TIMEOUT = 10
    REPORT_TIME = 300
    disable_title = True
    
    BLOCK = " | "
    PICK = " ⛏"
    COG = " ⚙"


class GlobalStats:
    """Global statistics shared across threads"""
    def __init__(self):
        self.accepted = 0
        self.rejected = 0
        self.blocks = 0
        self.hashrate = {}
        self.lock = Lock()
        self.start_time = time()
        
    def add_accepted(self, thread_id, value=1):
        with self.lock:
            self.accepted += value
            
    def add_rejected(self, thread_id, value=1):
        with self.lock:
            self.rejected += value
            
    def add_block(self):
        with self.lock:
            self.blocks += 1
            
    def update_hashrate(self, thread_id, hashrate):
        with self.lock:
            self.hashrate[thread_id] = hashrate
            
    def total_hashrate(self):
        with self.lock:
            return sum(self.hashrate.values())
    
    def get_stats(self):
        with self.lock:
            return self.accepted, self.rejected, self.blocks, self.total_hashrate()


def pretty_print(msg, state="success", sender="sys"):
    """Print formatted message"""
    if state == "success":
        color = Fore.GREEN
    elif state == "info":
        color = Fore.BLUE
    elif state == "error":
        color = Fore.RED
    else:
        color = Fore.YELLOW
    
    timestamp = datetime.now().strftime("%H:%M:%S")
    with printlock:
        print(f"{timestamp} [{sender}] {color}{msg}{Style.RESET_ALL}")


def get_prefix(val, symbol="H/s"):
    """Format hash rate"""
    if val >= 1_000_000:
        return f"{val/1_000_000:.2f} M{symbol}"
    elif val >= 1_000:
        return f"{val/1_000:.2f} k{symbol}"
    else:
        return f"{val:.2f} {symbol}"


class Client:
    """Socket client for pool communication"""
    _instance = None
    _lock = Lock()
    
    def __init__(self):
        self.socket = None
        
    @classmethod
    def get_instance(cls):
        with cls._lock:
            if cls._instance is None:
                cls._instance = cls()
            return cls._instance
    
    def connect(self, pool):
        try:
            self.socket = socket()
            self.socket.settimeout(Settings.SOC_TIMEOUT)
            self.socket.connect(pool)
            return True
        except Exception as e:
            pretty_print(f"Connection error: {e}", "error", "net")
            return False
    
    def send(self, msg):
        if self.socket:
            try:
                self.socket.sendall(str(msg).encode(Settings.ENCODING))
                return True
            except:
                return False
        return False
    
    def recv(self, limit=128):
        if self.socket:
            try:
                return self.socket.recv(limit).decode(Settings.ENCODING).rstrip("\n")
            except:
                return ""
        return ""
    
    def close(self):
        if self.socket:
            try:
                self.socket.close()
            except:
                pass
            self.socket = None


def fetch_pool():
    """Get best pool from server"""
    retry = 1
    while True:
        try:
            pretty_print("Searching for best node...", "info", "net")
            response = requests.get("https://server.duinocoin.com/getPool", timeout=5).json()
            if response.get("success"):
                pretty_print(f"Connected to node: {response.get('name', 'unknown')}", "info", "net")
                return (response["ip"], response["port"])
        except Exception as e:
            pretty_print(f"Error fetching pool: {e}, retrying...", "warning", "net")
        sleep(retry * 2)
        retry = min(retry + 1, 30)


def get_difficulty(diff_str):
    """Convert difficulty string to number"""
    diff_map = {"LOW": 2, "MEDIUM": 4, "NET": 6, "HIGH": 8}
    return diff_map.get(diff_str, 4)


class MinerThread(Thread):
    """Individual mining thread"""
    
    def __init__(self, thread_id, config, stats, pool):
        super().__init__(daemon=True)
        self.id = thread_id
        self.config = config
        self.stats = stats
        self.pool = pool
        self.running = True
        self.client = Client.get_instance()
        
    def stop(self):
        self.running = False
        
    def ducos1(self, last_hash, expected_hash, diff):
        """DUCO-S1 algorithm"""
        start_time = time()
        base_hash = sha1(last_hash.encode('ascii'))
        
        max_nonce = diff * 100
        for nonce in range(max_nonce + 1):
            temp_hash = base_hash.copy()
            temp_hash.update(str(nonce).encode('ascii'))
            result = temp_hash.hexdigest()
            
            if result == expected_hash:
                elapsed = time() - start_time
                hashrate = nonce / elapsed if elapsed > 0 else 0
                return nonce, hashrate
                
            # Optional efficiency delay
            if self.config.get("intensity", "100") != "100" and nonce % 5000 == 0:
                sleep(0.001)
                
        return 0, 0
    
    def run(self):
        """Main mining loop"""
        pretty_print(f"Thread {self.id} started", "success", f"cpu{self.id}")
        
        last_report = time()
        last_shares = 0
        
        # Get mining key
        mining_key = self.config.get("mining_key", "None")
        if mining_key != "None":
            import base64
            try:
                mining_key = base64.b64decode(mining_key).decode('utf-8')
            except:
                mining_key = "None"
        
        while self.running:
            try:
                # Connect to pool
                if not self.client.connect(self.pool):
                    sleep(5)
                    self.pool = fetch_pool()
                    continue
                
                # Get server version
                try:
                    server_ver = self.client.recv(5)
                    if self.id == 0:
                        pretty_print(f"Connected to server v{server_ver}", "success", "net")
                except:
                    pass
                
                while self.running:
                    try:
                        # Request job
                        job_req = f"JOB,{self.config['username']},{self.config['start_diff']},{mining_key}"
                        self.client.send(job_req)
                        
                        job_data = self.client.recv(256)
                        if not job_data:
                            break
                            
                        job_parts = job_data.split(Settings.SEPARATOR)
                        if len(job_parts) != 3:
                            sleep(1)
                            continue
                            
                        last_hash, target_hash, diff_str = job_parts
                        diff = int(diff_str)
                        
                        # Find nonce
                        start_time = time()
                        nonce, hashrate = self.ducos1(last_hash, target_hash, diff)
                        compute_time = time() - start_time
                        
                        if nonce > 0:
                            # Update stats
                            self.stats.update_hashrate(self.id, hashrate)
                            total_hashrate = self.stats.total_hashrate()
                            
                            # Submit result
                            result = f"{nonce},{hashrate},iOS_Miner,{self.config.get('identifier', 'iPhone')},"
                            self.client.send(result)
                            
                            # Get response
                            resp_start = time()
                            feedback = self.client.recv(32)
                            ping = (time() - resp_start) * 1000
                            
                            feedback_parts = feedback.split(Settings.SEPARATOR)
                            
                            if feedback_parts[0] == "GOOD":
                                self.stats.add_accepted(self.id)
                                accepted, rejected, blocks, _ = self.stats.get_stats()
                                
                                # Print success
                                percent = int(accepted/(accepted+rejected)*100) if accepted+rejected>0 else 0
                                with printlock:
                                    print(f"{datetime.now().strftime('%H:%M:%S')} [cpu{self.id}] "
                                          f"✅ Accepted {accepted}/{accepted+rejected} ({percent}%) "
                                          f"∙ {compute_time:.1f}s ∙ {get_prefix(hashrate)} "
                                          f"({get_prefix(total_hashrate)}) ∙ ping {int(ping)}ms")
                                
                            elif feedback_parts[0] == "BLOCK":
                                self.stats.add_accepted(self.id)
                                self.stats.add_block()
                                accepted, rejected, blocks, _ = self.stats.get_stats()
                                with printlock:
                                    print(f"{datetime.now().strftime('%H:%M:%S')} [cpu{self.id}] "
                                          f"⛓️ BLOCK FOUND! {accepted}/{accepted+rejected} shares")
                                    
                            elif feedback_parts[0] == "BAD":
                                self.stats.add_rejected(self.id)
                                reason = feedback_parts[1] if len(feedback_parts) > 1 else "unknown"
                                with printlock:
                                    print(f"{datetime.now().strftime('%H:%M:%S')} [cpu{self.id}] "
                                          f"❌ Rejected: {reason}")
                            
                            # Periodic report
                            if self.id == 0 and time() - last_report >= Settings.REPORT_TIME:
                                accepted, rejected, blocks, total_hr = self.stats.get_stats()
                                elapsed = time() - self.stats.start_time
                                shares_this_period = accepted - last_shares
                                last_shares = accepted
                                last_report = time()
                                
                                with printlock:
                                    print(f"\n📊 Report: {shares_this_period} shares in {Settings.REPORT_TIME}s, "
                                          f"avg {get_prefix(total_hr)}, total: {accepted}/{accepted+rejected} "
                                          f"({blocks} blocks), uptime: {int(elapsed)}s\n")
                        
                    except Exception as e:
                        if self.running:
                            pretty_print(f"Error in mining loop: {e}", "error", f"cpu{self.id}")
                        break
                        
            except Exception as e:
                if self.running:
                    pretty_print(f"Connection lost: {e}, reconnecting...", "warning", f"net{self.id}")
                sleep(5)
                self.pool = fetch_pool()
        
        pretty_print(f"Thread {self.id} stopped", "info", f"cpu{self.id}")


def load_config():
    """Load or create configuration"""
    if not Path(Settings.DATA_DIR).is_dir():
        os.makedirs(Settings.DATA_DIR)
    
    config_file = Settings.DATA_DIR + Settings.SETTINGS_FILE
    
    if not Path(config_file).is_file():
        print("\n" + "="*50)
        print("   First time setup")
        print("="*50)
        
        username = input("Duino-Coin username: ").strip()
        if not username:
            username = "Nam2010"
        
        mining_key = input("Mining key (press Enter if none): ").strip()
        if mining_key:
            import base64
            mining_key = base64.b64encode(mining_key.encode()).decode()
        else:
            mining_key = "None"
        
        threads = input(f"Threads (1-4, default 2): ").strip()
        threads = int(threads) if threads.isdigit() else 2
        threads = max(1, min(4, threads))
        
        print("\nDifficulty:")
        print("1 - LOW (easier, less reward)")
        print("2 - MEDIUM (balanced)")
        print("3 - NET (harder, more reward)")
        diff_choice = input("Choose (1-3, default 2): ").strip()
        if diff_choice == "1":
            start_diff = "LOW"
        elif diff_choice == "3":
            start_diff = "NET"
        else:
            start_diff = "MEDIUM"
        
        rig_id = input("Rig name (press Enter for none): ").strip()
        if not rig_id:
            rig_id = "iPhone"
        
        config = {
            "username": username,
            "mining_key": mining_key,
            "threads": str(threads),
            "start_diff": start_diff,
            "identifier": rig_id,
            "intensity": "100"
        }
        
        configparser["PC Miner"] = config
        with open(config_file, "w") as f:
            configparser.write(f)
        
        print("Configuration saved!\n")
    
    configparser.read(config_file)
    return dict(configparser["PC Miner"])


def signal_handler(sig, frame):
    """Handle Ctrl+C"""
    print("\n🛑 Stopping miner...")
    for thread in threads:
        thread.stop()
    sys.exit(0)


# Main execution
if __name__ == "__main__":
    signal(SIGINT, signal_handler)
    
    # Load config
    config = load_config()
    
    # Print banner
    print("\n" + "="*50)
    print("   🦀 Duino-Coin iOS Miner v" + str(Settings.VER))
    print("="*50)
    print(f"Username:   {config['username']}")
    print(f"Difficulty: {config['start_diff']}")
    print(f"Rig:        {config.get('identifier', 'iPhone')}")
    print(f"Threads:    {config['threads']}")
    print("="*50)
    print("✅ Miner started! Press Ctrl+C to stop.\n")
    
    # Get pool
    pool = fetch_pool()
    
    # Initialize stats
    stats = GlobalStats()
    
    # Start mining threads
    threads = []
    for i in range(int(config['threads'])):
        t = MinerThread(i, config, stats, pool)
        t.start()
        threads.append(t)
    
    # Keep main thread alive
    try:
        while True:
            sleep(1)
    except KeyboardInterrupt:
        signal_handler(None, None)
