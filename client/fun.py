# This file stores functions for the project 'Calculus'.

import socket

def isValidIP(address):
    try:
        socket.inet_pton(socket.AF_INET, address)
    except AttributeError:
        try:
            socket.inet_aotn(address)
        except socket.error:
            return False
        return address.count('.') == 3
    except socket.error:
        return False

    return True

