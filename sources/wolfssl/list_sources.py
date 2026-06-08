import os
import os.path


def list_sources(subdir, blacklist=[]):
    sources = []
    for fname in os.listdir(subdir):
        name, ext = os.path.splitext(fname)
        if ext != '.c':
            continue
        ignore = False
        for bl in blacklist:
            if name.startswith(bl):
                ignore = True
                break
        if not ignore:
            sources.append(os.path.join(subdir, fname))
    sources.sort()
    return sources


def all_sources():
    return (
        list_sources("./src", ['ssl_', 'x509_', 'conf', 'bio']) +
        list_sources("./wolfcrypt/src", ['misc', 'evp'])
    )

src_str = """# Begin Source File

SOURCE={}
# End Source File"""

for src in all_sources():
    print(src_str.format(src.replace("/", "\\")))
