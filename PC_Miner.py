#!/usr/bin/env python3
"""
Duino-Coin Official PC Miner 4.3 - iOS Compatible Version
Removed incompatible modules: cpufinfo, psutil, pypresence, fasthash
"""

from time import time, sleep, strptime, ctime
from hashlib import sha1
from socket import socket

from multiprocessing import cpu_count, current_process
from multiprocessing import Process, Manager, Semaphore
from threading import Thread, Lock
from datetime import datetime
from random import randint

from os import execl, mkdir, _exit
from os import name as osname
from os import system as ossystem 
from subprocess import DEVNULL, Popen, check_call, PIPE
import sys
import os
import json
import traceback
import urllib.parse

from pathlib import Path
from re import sub
from random import choice
from platform import machine as osprocessor
from platform import python_version_tuple
from platform import python_version

from signal import SIGINT, signal
from locale import getdefaultlocale
from configparser import ConfigParser

debug = "n"
running_on_rpi = False
configparser = ConfigParser()
printlock = Lock()

# Python <3.5 check
if int(python_version_tuple()[0]) < 3 or int(python_version_tuple()[1]) < 6:
    print("Your Python version is too old. Duino-Coin Miner requires version 3.6 or above.")
    sys.exit(1)


def handler(signal_received, frame):
    """Nicely handle CTRL+C exit"""
    if current_process().name == "MainProcess":
        print("\n🛑 Stopping miner... Goodbye!")
    
    if sys.platform == "win32":
        _exit(0)
    else: 
        Popen("kill $(ps aux | grep PC_Miner | awk '{print $2}')",
              shell=True, stdout=PIPE)


def debug_output(text: str):
    if debug == 'y':
        print(datetime.now().strftime('%H:%M:%S.%f') + f' DEBUG: {text}')


def install(package):
    """Automatically installs python pip package"""
    try:
        subprocess.check_call([sys.executable, '-m', 'pip', 'install', package])
    except:
        pass


try:
    import requests
except ModuleNotFoundError:
    print("Installing requests...")
    install("requests")
    import requests

try:
    from colorama import Back, Fore, Style, init
    init(autoreset=True)
except ModuleNotFoundError:
    print("Installing colorama...")
    install("colorama")
    from colorama import Back, Fore, Style, init
    init(autoreset=True)


class Settings:
    """Class containing default miner and server settings"""
    ENCODING = "UTF8"
    SEPARATOR = ","
    VER = 4.3
    DATA_DIR = "Duino-Coin PC Miner " + str(VER)
    TRANSLATIONS = ("https://raw.githubusercontent.com/"
                    + "revoxhere/"
                    + "duino-coin/master/Resources/"
                    + "PC_Miner_langs.json")
    TRANSLATIONS_FILE = "/Translations.json"
    SETTINGS_FILE = "/Settings.cfg"
    TEMP_FOLDER = "Temp"

    SOC_TIMEOUT = 10
    REPORT_TIME = 300
    DONATE_LVL = 0
    RASPI_LEDS = "y"
    RASPI_CPU_IOT = "y"
    disable_title = False

    try:
        BLOCK = " ‖ "
        "‖".encode(sys.stdout.encoding)
    except:
        BLOCK = " | "
    PICK = ""
    COG = " @"
    if (os.name != "nt"):
        try:
            "⛏ ⚙".encode(sys.stdout.encoding)
            PICK = " ⛏"
            COG = " ⚙"
        except UnicodeEncodeError:
            PICK = ""
            COG = " @"


def title(title: str):
    if not Settings.disable_title:
        if osname == 'nt':
            ossystem('title ' + title)
        else:
            try:
                print('\33]0;' + title + '\a', end='')
                sys.stdout.flush()
            except Exception as e:
                debug_output("Error setting title: " + str(e))
                Settings.disable_title = True


def check_updates():
    """Check for updates (disabled on iOS)"""
    pass  # Disabled for iOS compatibility


class Algorithms:
    """Class containing algorithms used by the miner"""
    def DUCOS1(last_h: str, exp_h: str, diff: int, eff: int):
        time_start = time()
        base_hash = sha1(last_h.encode('ascii'))

        for nonce in range(100 * diff + 1):
            temp_h = base_hash.copy()
            temp_h.update(str(nonce).encode('ascii'))
            d_res = temp_h.hexdigest()

            if eff != 0:
                if nonce % 5000 == 0:
                    sleep(eff / 100)

            if d_res == exp_h:
                time_elapsed = time() - time_start
                if time_elapsed > 0:
                    hashrate = nonce / time_elapsed
                else:
                    return [nonce, 0]
                return [nonce, hashrate]

        return [0, 0]


class Client:
    """Class helping to organize socket connections"""
    s = None
    
    def connect(pool: tuple):
        Client.s = socket()
        Client.s.settimeout(Settings.SOC_TIMEOUT)
        Client.s.connect((pool))

    def send(msg: str):
        sent = Client.s.sendall(str(msg).encode(Settings.ENCODING))
        return sent

    def recv(limit: int = 128):
        data = Client.s.recv(limit).decode(Settings.ENCODING).rstrip("\n")
        return data

    def fetch_pool(retry_count=1):
        """Fetches the best pool from the /getPool API endpoint"""
        while True:
            if retry_count > 60:
                retry_count = 60

            try:
                pretty_print("Searching for best node...", "info", "net0")
                response = requests.get(
                    "https://server.duinocoin.com/getPool",
                    timeout=Settings.SOC_TIMEOUT).json()

                if response.get("success") == True:
                    pretty_print("Connecting to node: " + response.get("name", "unknown"), "info", "net0")
                    return (response["ip"], response["port"])
                else:
                    pretty_print(f"Warning: {response.get('message', 'unknown error')}, retrying...",
                                 "warning", "net0")
            except Exception as e:
                pretty_print(f"Node picker error: {e}, retrying in {retry_count*2}s", "error", "net0")
            sleep(retry_count * 2)
            retry_count += 1


class Donate:
    def load(donation_level):
        pass  # Disabled for iOS
    
    def start(donation_level):
        if donation_level <= 0:
            pretty_print(
                Fore.YELLOW + "You are mining on the free network. "
                + "Consider donating to support the project: "
                + Fore.GREEN + 'https://duinocoin.com/donate',
                'warning', 'sys0')


def get_prefix(symbol: str, val: float, accuracy: int):
    """Format hash rate with prefix"""
    if val >= 1_000_000_000:
        val = str(round((val / 1_000_000_000), accuracy)) + " G"
    elif val >= 1_000_000:
        val = str(round((val / 1_000_000), accuracy)) + " M"
    elif val >= 1_000:
        val = str(round((val / 1_000))) + " k"
    else:
        val = str(round(val)) + " "
    return val + symbol


def periodic_report(start_time, end_time, shares, blocks, hashrate, uptime):
    """Displays nicely formatted uptime stats"""
    seconds = round(end_time - start_time)
    pretty_print(
        f"📊 Periodic mining report: {seconds}s, {shares} shares, "
        f"{blocks} blocks, {get_prefix('H/s', hashrate, 2)} avg, "
        f"uptime: {uptime}", "success")


def calculate_uptime(start_time):
    """Returns seconds, minutes or hours passed since timestamp"""
    uptime = time() - start_time
    if uptime >= 7200:
        return f"{int(uptime // 3600)}h"
    elif uptime >= 3600:
        return f"{int(uptime // 3600)}h"
    elif uptime >= 120:
        return f"{int(uptime // 60)}m"
    elif uptime >= 60:
        return f"{int(uptime // 60)}m"
    else:
        return f"{int(uptime)}s"


def pretty_print(msg: str = None, state: str = "success", sender: str = "sys0", print_queue=None):
    """Produces nicely formatted CLI output"""
    if sender.startswith("net"):
        bg_color = Back.BLUE
    elif sender.startswith("cpu"):
        bg_color = Back.YELLOW
    elif sender.startswith("sys"):
        bg_color = Back.GREEN
    else:
        bg_color = Back.RESET

    if state == "success":
        fg_color = Fore.GREEN
    elif state == "info":
        fg_color = Fore.BLUE
    elif state == "error":
        fg_color = Fore.RED
    else:
        fg_color = Fore.YELLOW

    output = (datetime.now().strftime("%H:%M:%S ") + 
              bg_color + " " + sender + " " + Back.RESET + " " + 
              fg_color + str(msg).strip())
    
    if print_queue is not None:
        print_queue.append(output)
    else:
        print(output)


def share_print(id, type, accept, reject, thread_hashrate, total_hashrate,
                computetime, diff, ping, back_color, reject_cause=None, print_queue=None):
    """Produces nicely formatted CLI output for shares"""
    thread_hashrate = get_prefix("H/s", thread_hashrate, 2)
    total_hashrate = get_prefix("H/s", total_hashrate, 1)
    diff = get_prefix("", int(diff), 0)

    if type == "accept":
        share_str = "✅ Accepted"
        fg_color = Fore.GREEN
    elif type == "block":
        share_str = "⛓️ Block found!"
        fg_color = Fore.YELLOW
    else:
        share_str = "❌ Rejected"
        if reject_cause:
            share_str += f"({reject_cause}) "
        fg_color = Fore.RED

    output = (datetime.now().strftime("%H:%M:%S ") +
              back_color + f" cpu{id} " + Back.RESET + fg_color +
              share_str + Fore.RESET + f"{accept}/{(accept + reject)}" +
              f" ({int(accept/(accept+reject)*100) if accept+reject>0 else 0}%)" +
              f" ∙ {computetime:.1f}s ∙ {thread_hashrate} ({total_hashrate})" +
              f" ∙ diff {diff} ∙ ping {int(ping)}ms")
    
    if print_queue is not None:
        print_queue.append(output)
    else:
        print(output)


def print_queue_handler(print_queue):
    """Prevents broken console logs with many threads"""
    while True:
        if len(print_queue):
            message = print_queue[0]
            with printlock:
                print(message)
            print_queue.pop(0)
        sleep(0.01)


def get_string(string_name):
    """Gets a string from the language file"""
    if string_name in lang_file.get(lang, {}):
        return lang_file[lang][string_name]
    elif string_name in lang_file.get("english", {}):
        return lang_file["english"][string_name]
    else:
        return string_name


def has_mining_key(username):
    try:
        response = requests.get(
            "https://server.duinocoin.com/mining_key?u=" + username,
            timeout=10
        ).json()
        return response.get("has_key", False)
    except:
        return False


def check_mining_key(user_settings):
    """Check and validate mining key"""
    if user_settings["mining_key"] != "None":
        try:
            import base64
            key = base64.b64decode(user_settings["mining_key"]).decode('utf-8')
        except:
            key = ""
    else:
        key = ""

    try:
        response = requests.get(
            "https://server.duinocoin.com/mining_key?u=" + user_settings["username"] + ("&k=" + key if key else ""),
            timeout=Settings.SOC_TIMEOUT
        ).json()

        if response.get("success") and not response.get("has_key"):
            user_settings["mining_key"] = "None"
            with open(Settings.DATA_DIR + Settings.SETTINGS_FILE, "w") as configfile:
                configparser.write(configfile)
    except:
        pass


class Miner:
    def greeting():
        diff_str = "NET"
        if user_settings["start_diff"] == "LOW":
            diff_str = "LOW"
        elif user_settings["start_diff"] == "MEDIUM":
            diff_str = "MEDIUM"

        print("\n" + "=" * 50)
        print("   🦀 Duino-Coin Python Miner v" + str(Settings.VER))
        print("=" * 50)
        print("https://github.com/revoxhere/duino-coin")
        print("CPU: " + str(user_settings["threads"]) + " threads")
        print("Donation level: " + str(user_settings["donate"]))
        print("Algorithm: " + user_settings["algorithm"] + " " + diff_str)
        if user_settings["identifier"] != "None":
            print("Rig identifier: " + user_settings["identifier"])
        print("Config: " + Settings.DATA_DIR + Settings.SETTINGS_FILE)
        print("Hello, " + user_settings["username"] + "!\n")

    def preload():
        """Creates needed directories and files for the miner"""
        global lang_file, lang

        if not Path(Settings.DATA_DIR).is_dir():
            mkdir(Settings.DATA_DIR)

        if not Path(Settings.DATA_DIR + Settings.TRANSLATIONS_FILE).is_file():
            try:
                with open(Settings.DATA_DIR + Settings.TRANSLATIONS_FILE, "wb") as f:
                    f.write(requests.get(Settings.TRANSLATIONS, timeout=Settings.SOC_TIMEOUT).content)
            except:
                pass

        try:
            with open(Settings.DATA_DIR + Settings.TRANSLATIONS_FILE, "r", encoding=Settings.ENCODING) as file:
                lang_file = json.load(file)
        except:
            lang_file = {"english": {}}
            lang = "english"
            return

        try:
            if not Path(Settings.DATA_DIR + Settings.SETTINGS_FILE).is_file():
                locale = getdefaultlocale()[0] or "en"
                if locale.startswith("es"):
                    lang = "spanish"
                elif locale.startswith("pl"):
                    lang = "polish"
                elif locale.startswith("fr"):
                    lang = "french"
                elif locale.startswith("zh"):
                    lang = "chinese_simplified"
                elif locale.startswith("vi"):
                    lang = "vietnamese"
                else:
                    lang = "english"
            else:
                configparser.read(Settings.DATA_DIR + Settings.SETTINGS_FILE)
                lang = configparser["PC Miner"].get("language", "english")
        except:
            lang = "english"

    def load_cfg():
        """Loads miner settings file or starts the config tool"""
        if not Path(Settings.DATA_DIR + Settings.SETTINGS_FILE).is_file():
            print("\n=== Basic Configuration Tool ===")
            print("Config will be saved to: " + Settings.DATA_DIR + Settings.SETTINGS_FILE)
            print("Don't have an account? Register at: https://duinocoin.com")
            
            username = input("Username: ").strip()
            if not username:
                username = choice(["revox", "Bilaboz"])
            
            mining_key = "None"
            if has_mining_key(username):
                mining_key = input("Mining key (if you have one): ").strip()
                if mining_key:
                    import base64
                    mining_key = base64.b64encode(mining_key.encode("utf-8")).decode('utf-8')
                else:
                    mining_key = "None"
            
            intensity = input("Intensity (1-100, default 95): ").strip()
            if not intensity:
                intensity = 95
            else:
                intensity = int(intensity)
                intensity = max(1, min(100, intensity))
            
            threads = input(f"Threads (1-{cpu_count()}, default {cpu_count()}): ").strip()
            if not threads:
                threads = cpu_count()
            else:
                threads = int(threads)
                threads = max(1, min(16, threads))
            
            print("Difficulty options:")
            print("1 - LOW")
            print("2 - MEDIUM")
            print("3 - NET")
            diff_choice = input("Choose difficulty (1-3, default 2): ").strip()
            if diff_choice == "1":
                start_diff = "LOW"
            elif diff_choice == "3":
                start_diff = "NET"
            else:
                start_diff = "MEDIUM"
            
            rig_id = input("Rig identifier (press Enter for none): ").strip()
            if not rig_id:
                rig_id = "None"
            
            donation_level = input("Donation level (0-5, default 1): ").strip()
            if not donation_level:
                donation_level = 1
            else:
                donation_level = int(donation_level)
                donation_level = max(0, min(5, donation_level))
            
            configparser["PC Miner"] = {
                "username": username,
                "mining_key": mining_key,
                "intensity": str(intensity),
                "threads": str(threads),
                "start_diff": start_diff,
                "donate": str(donation_level),
                "identifier": rig_id,
                "algorithm": "DUCO-S1",
                "language": lang,
                "soc_timeout": str(Settings.SOC_TIMEOUT),
                "report_sec": str(Settings.REPORT_TIME),
                "raspi_leds": Settings.RASPI_LEDS,
                "raspi_cpu_iot": Settings.RASPI_CPU_IOT,
                "discord_rp": "n"}

            with open(Settings.DATA_DIR + Settings.SETTINGS_FILE, "w") as configfile:
                configparser.write(configfile)
                print("Configuration saved!")

        configparser.read(Settings.DATA_DIR + Settings.SETTINGS_FILE)
        return configparser["PC Miner"]

    def m_connect(id, pool):
        retry_count = 0
        while True:
            try:
                if retry_count > 3:
                    pool = Client.fetch_pool()
                    retry_count = 0

                Client.connect(pool)
                POOL_VER = Client.recv(5)

                if id == 0:
                    Client.send("MOTD")
                    motd = Client.recv(512)
                    pretty_print("MOTD: " + motd, "success", "net" + str(id))
                    pretty_print(f"Connected to server v{POOL_VER}, {pool[0]}", "success", "net" + str(id))
                break
            except Exception as e:
                pretty_print(f'Connection error: {e}', 'error', 'net0')
                retry_count += 1
                sleep(10)

    def mine(id: int, user_settings: dict, blocks, pool, accept, reject, hashrate, single_miner_id, print_queue):
        """Main mining function"""
        pretty_print(f"Mining thread {id} starting with intensity {user_settings['intensity']}%", 
                     "success", "sys"+str(id), print_queue=print_queue)

        last_report = time()
        r_shares, last_shares = 0, 0
        while True:
            accept.value = 0
            reject.value = 0
            try:
                Miner.m_connect(id, pool)
                while True:
                    try:
                        key = user_settings["mining_key"]
                        if key != "None":
                            import base64
                            key = base64.b64decode(key).decode('utf-8')

                        while True:
                            job_req = "JOB"
                            Client.send(job_req + Settings.SEPARATOR +
                                       str(user_settings["username"]) + Settings.SEPARATOR +
                                       str(user_settings["start_diff"]) + Settings.SEPARATOR +
                                       str(key))

                            job = Client.recv().split(Settings.SEPARATOR)
                            if len(job) == 3:
                                break
                            else:
                                pretty_print("Node message: " + str(job), "warning", print_queue=print_queue)
                                sleep(3)

                        while True:
                            time_start = time()
                            back_color = Back.YELLOW

                            eff = 0
                            eff_setting = int(user_settings["intensity"])
                            if 99 > eff_setting >= 90:
                                eff = 0.005
                            elif 90 > eff_setting >= 70:
                                eff = 0.1
                            elif 70 > eff_setting >= 50:
                                eff = 0.8
                            elif 50 > eff_setting >= 30:
                                eff = 1.8
                            elif 30 > eff_setting >= 1:
                                eff = 3

                            result = Algorithms.DUCOS1(job[0], job[1], int(job[2]), eff)
                            computetime = time() - time_start

                            hashrate[id] = result[1]
                            total_hashrate = sum(hashrate.values())

                            while True:
                                Client.send(f"{result[0]}" + Settings.SEPARATOR +
                                           f"{result[1]}" + Settings.SEPARATOR +
                                           "iOS Python Miner" + Settings.SEPARATOR +
                                           user_settings['identifier'] + Settings.SEPARATOR +
                                           Settings.SEPARATOR + f"{single_miner_id}")

                                time_start = time()
                                feedback = Client.recv().split(Settings.SEPARATOR)
                                ping = (time() - time_start) * 1000

                                if feedback[0] == "GOOD":
                                    accept.value += 1
                                    share_print(id, "accept", accept.value, reject.value,
                                               hashrate[id], total_hashrate,
                                               computetime, job[2], ping,
                                               back_color, print_queue=print_queue)

                                elif feedback[0] == "BLOCK":
                                    accept.value += 1
                                    blocks.value += 1
                                    share_print(id, "block", accept.value, reject.value,
                                               hashrate[id], total_hashrate,
                                               computetime, job[2], ping,
                                               back_color, print_queue=print_queue)

                                elif feedback[0] == "BAD":
                                    reject.value += 1
                                    share_print(id, "reject", accept.value, reject.value,
                                               hashrate[id], total_hashrate,
                                               computetime, job[2], ping,
                                               back_color, feedback[1] if len(feedback) > 1 else None,
                                               print_queue=print_queue)

                                title(f"Duino-Coin Miner v{Settings.VER} - "
                                      f"{accept.value}/{(accept.value + reject.value)} accepted shares")

                                if id == 0:
                                    end_time = time()
                                    elapsed_time = end_time - last_report
                                    if elapsed_time >= int(user_settings["report_sec"]):
                                        r_shares = accept.value - last_shares
                                        uptime = calculate_uptime(mining_start_time)
                                        periodic_report(last_report, end_time, r_shares, blocks.value,
                                                        sum(hashrate.values()), uptime)
                                        last_report = time()
                                        last_shares = accept.value
                                break
                            break
                    except Exception as e:
                        pretty_print(f"Error while mining: {e}", "error", "net" + str(id), print_queue=print_queue)
                        sleep(5)
                        break
            except Exception as e:
                pretty_print(f"Error while mining: {e}", "error", "net" + str(id), print_queue=print_queue)


# Main execution
Miner.preload()
p_list = []
mining_start_time = time()

if __name__ == "__main__":
    from multiprocessing import freeze_support
    freeze_support()
    signal(SIGINT, handler)
    title(f"Duino-Coin Miner v{Settings.VER})")

    if sys.platform == "win32":
        os.system('')

    accept = Manager().Value("i", 0)
    reject = Manager().Value("i", 0)
    blocks = Manager().Value("i", 0)
    hashrate = Manager().dict()
    print_queue = Manager().list()
    Thread(target=print_queue_handler, args=[print_queue], daemon=True).start()

    user_settings = Miner.load_cfg()
    Miner.greeting()

    if "raspi_leds" not in user_settings:
        user_settings["raspi_leds"] = "y"
    if "raspi_cpu_iot" not in user_settings:
        user_settings["raspi_cpu_iot"] = "y"

    try:
        check_mining_key(user_settings)
    except Exception as e:
        print("Error checking mining key:", e)

    Donate.start(int(user_settings.get("donate", 0)))

    single_miner_id = randint(0, 2811)
    threads = int(user_settings.get("threads", cpu_count()))
    threads = min(threads, cpu_count(), 16)

    fastest_pool = Client.fetch_pool()

    for i in range(threads):
        p = Process(target=Miner.mine,
                    args=[i, user_settings, blocks,
                          fastest_pool, accept, reject,
                          hashrate, single_miner_id, 
                          print_queue])
        p_list.append(p)
        p.start()

    for p in p_list:
        p.join()
