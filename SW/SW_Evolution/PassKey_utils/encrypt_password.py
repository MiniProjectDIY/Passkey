from Crypto.Cipher import AES
from Crypto.Util.Padding import pad
import base64
aes_key=bytes.fromhex("415034543035340B00B03048EEEEEEEE")

iv = bytes([i for i in range(16)]) 

plaintext = b"YOUR_REAL_PASSWORD"
cipher = AES.new(aes_key, AES.MODE_CBC, iv)
ciphertext = cipher.encrypt(pad(plaintext, 16))

output = base64.b64encode(iv + ciphertext).decode()
print("Base64:", output)