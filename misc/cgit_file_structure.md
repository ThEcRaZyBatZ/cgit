## custom git structure

```
.cgit/
 ├── objects/
 ├── HEAD
 ├── logs/
 |    └── master
 └── refs/
      └── master
```

```
.cgit/HEAD stores nothing yet, it will after we address the gaps
.cgit/objects/ stores all the objects
.cgit/logs/master stores all the log entries 
.cgit/regfs/master stores the latest commit
```