

# Usage:


### 1. Topic demo:

``` python
# Publisher test 
python  ipc_demo.py pub  --topic asd --transport socket
# Subcriber test
python  ipc_demo.py sub  --topic asd --transport socket
```

### 2. Server demo:

``` python
# Publisher test 
python  ipc_demo.py server  --topic asd --transport socket
# Subcriber test
python  ipc_demo.py client  --topic asd --transport socket
```