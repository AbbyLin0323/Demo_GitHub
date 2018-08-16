
xt-dumpelf --base=0x40000000  --size=0x1000 --width=32 --little-endian --default=0x00000000 --full D:\Xplorer-6.0.1-workspaces\overlaylib1\bin\viatie\Release\overlaylib1 > .\overlaylib1.tmp
.\asc_to_bin .\overlaylib1.tmp .\overlaylib1.data

xt-dumpelf --base=0x40000000  --size=0x1000 --width=32 --little-endian --default=0x00000000 --full D:\Xplorer-6.0.1-workspaces\overlaylib2\bin/viatie/Release/overlaylib2 > .\overlaylib2.tmp
.\asc_to_bin .\overlaylib2.tmp .\overlaylib2.data