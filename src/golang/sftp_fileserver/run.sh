
# generate a private key if not exist: `ssh-keygen -t rsa -f id_rsa`
nohup go run main.go &

sleep 3

sftp -P 2023 testuser@localhost