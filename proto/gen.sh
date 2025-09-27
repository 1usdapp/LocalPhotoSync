
python3 gen_msgid.py csmsg.proto msgid.proto

protoc  --cpp_out=. *.proto
