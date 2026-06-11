# sql

SQL queries can go here.  

Lux tries to upload the convention of using this folder for SQL files that are too big to be specified inline via:

```
local res = db.exec{
	string = [[ SELECT * FROM some_table ]]
}
```


