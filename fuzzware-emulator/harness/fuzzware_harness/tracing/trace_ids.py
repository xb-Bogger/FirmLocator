import logging

from ..exit import do_exit

logger = logging.getLogger("emulator")

event_id = -1
event_id_limit = 0

def next_event_id(uc):
    global event_id
    global event_id_limit
    event_id += 1
    if event_id_limit != 0 and event_id >= event_id_limit:
        logger.info("[*] Event id limit reached, exiting")
        do_exit(uc, 0)
    return event_id

def set_trace_id_limit(limit):
    global event_id_limit
    event_id_limit = limit
