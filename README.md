# txsfix
txsfix - a ngc winning eleven 6 txs fixer

this fixed the edited textures that you have patched with we2bmp.
it's used in combination with the afs explorer and the zlib manager.

usage:
1. load data.txs in the zlib manager
2. set compression to max
3. use compress to replace one graphic (more is not supported by txsfix)
4. write it back into the afs file
5. export the data.txs again
6. now fix the data.txs -> txsfix -i data.txs -p data_patched.txs
7. import the data_patched.txs instead of the data.txs
8. take a look with the zlib manager again, just in case
