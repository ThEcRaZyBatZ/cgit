# CGIT Object Formats

## Blob (file)

blob <size>\0<file_bytes>


Hash:

SHA1("blob <size>\0" + file_bytes)


Stored at:

.cgit/objects/ab/cdef...


---

## Tree Object

### File Entry

100644 <filename>\0<20-byte blob hash>

### Directory Entry

40000 <dirname>\0<20-byte tree hash>


### Tree Format

```
tree <payload_size>\0<tree_payload>
```


Where:

tree_payload = [entry1][entry2][entry3]...


Hash:

SHA1("tree <payload_size>\0" + tree_payload)


---

## Commit Object

```
commit <content_size>\0
tree_hash (20 bytes)
parent_hash (20 bytes, optional)
message
```


Hash:

SHA1("commit <content_size>\0" + tree_hash + parent_hash + message)


---

## Object Storage Layout

```
.cgit/
 ├── objects/
 ├── HEAD
 ├── logs/
 |    └── master
 └── refs/
      └── master
```

---

