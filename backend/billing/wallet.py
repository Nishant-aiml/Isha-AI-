"""
Enterprise Pay-As-You-Go Billing & Wallet Ledger (Python / Desktop & Web).
Ensures zero-bleed cloud API consumption while keeping offline SLMs and generators 100% free.
"""

import os
import json
import hashlib
import hmac
from typing import Dict, Any, Tuple

LEDGER_FILE = os.path.join(os.path.dirname(os.path.dirname(__file__)), "data", "billing_ledger.json")

# Costs in credits
COST_CLOUD_CHAT = 1
COST_CLOUD_IMAGE = 5
COST_CLOUD_DECK = 10
COST_CLOUD_WEB = 10
COST_CLOUD_VIDEO = 15
COST_OFFLINE = 0

INITIAL_STARTER_CREDITS = 25
SECRET_SALT = b"isha_ai_desktop_ledger_salt_2026"


class InsufficientBalanceException(Exception):
    def __init__(self, required: int, available: int):
        super().__init__(f"Insufficient cloud credits ({available} available, {required} required). Recharge or use Free Offline Mode.")
        self.required = required
        self.available = available


def _compute_hmac(balance: int) -> str:
    msg = f"balance:{balance}".encode("utf-8")
    return hmac.new(SECRET_SALT, msg, hashlib.sha256).hexdigest()


def _load_ledger() -> Dict[str, Any]:
    os.makedirs(os.path.dirname(LEDGER_FILE), exist_ok=True)
    if not os.path.exists(LEDGER_FILE):
        initial = {
            "balance": INITIAL_STARTER_CREDITS,
            "signature": _compute_hmac(INITIAL_STARTER_CREDITS),
            "total_spent": 0
        }
        with open(LEDGER_FILE, "w", encoding="utf-8") as f:
            json.dump(initial, f, indent=2)
        return initial

    try:
        with open(LEDGER_FILE, "r", encoding="utf-8") as f:
            data = json.load(f)
            balance = data.get("balance", 0)
            sig = data.get("signature", "")
            if sig == _compute_hmac(balance):
                return data
            else:
                # Tampered or corrupted: reset safely
                data["balance"] = 0
                data["signature"] = _compute_hmac(0)
                return data
    except Exception:
        return {"balance": 0, "signature": _compute_hmac(0), "total_spent": 0}


def _save_ledger(data: Dict[str, Any]):
    os.makedirs(os.path.dirname(LEDGER_FILE), exist_ok=True)
    data["signature"] = _compute_hmac(data.get("balance", 0))
    with open(LEDGER_FILE, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2)


def get_credit_balance() -> int:
    ledger = _load_ledger()
    return ledger.get("balance", 0)


def can_afford_cloud(cost: int, has_byok: bool = False) -> bool:
    if has_byok:
        return True
    return get_credit_balance() >= cost


def deduct_cloud_credits(cost: int, description: str = "", has_byok: bool = False) -> Tuple[bool, int]:
    if cost == 0:
        return True, get_credit_balance()
    if has_byok:
        return True, get_credit_balance()

    ledger = _load_ledger()
    balance = ledger.get("balance", 0)
    if balance < cost:
        raise InsufficientBalanceException(cost, balance)

    ledger["balance"] = balance - cost
    ledger["total_spent"] = ledger.get("total_spent", 0) + cost
    _save_ledger(ledger)
    return True, ledger["balance"]


def recharge_wallet(amount: int) -> int:
    ledger = _load_ledger()
    ledger["balance"] = ledger.get("balance", 0) + amount
    _save_ledger(ledger)
    return ledger["balance"]
