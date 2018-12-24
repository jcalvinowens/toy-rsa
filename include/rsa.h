#pragma once

struct rsa_key;

int rsa_cipher_test(int bits);
void rsa_generate_keypair(struct rsa_key **pub, struct rsa_key **priv, int bits);
