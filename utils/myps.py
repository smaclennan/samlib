#!/usr/bin/python

import os, re, sys, pwd

# Who are you?
you = os.getuid()

# Pre-compile the regular expressions
pats  = [ re.compile(r'Name:\s+(\w+)'),
          re.compile(r'State:\s+(.)'),
          re.compile(r'Uid:\s+([0-9]+)') ]

def iskernelthread (fname):
    "Kernel threads have a zero length cmdline."
    fp = open("/proc/" + fname + "/cmdline", "r")
    n = fp.read(4)
    fp.close()
    if len(n) == 0:
        return True
    return False

uidcache = [ [0, 'root'], [you, 'you'] ]

def lookup_uid (uid):
    global uidcache

    for u in uidcache:
        if u[0] == uid:
            return u[1]

    try:
        pw = pwd.getpwuid(uid)[0]
        uidcache.append([uid, pw])
        return pw
    except:
        return str(uid)

def dump_ps (pid):
    result = []
    with open("/proc/" + pid + "/status") as fp:
        for line in fp:
            for pat in pats:
                m = pat.match(line)
                if m:
                    result.append(m.group(1))
                    break
            if len(result) == 3: break # optimization

    print "%5s  %c  %-8.8s  %s" % (pid, result[1], lookup_uid(int(result[2])), result[0])

for fname in os.listdir("/proc"):
    if not re.match('[0-9]+$', fname): continue
    if iskernelthread(fname): continue
    dump_ps(fname)
