import os
Import("env")

def before_upload(source, target, env):
    print("Uploading LittleFS files...")
    os.system("pio run --target uploadfs")

env.AddPreAction("upload", before_upload)
