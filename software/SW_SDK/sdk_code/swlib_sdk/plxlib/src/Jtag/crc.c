static unsigned int crc_calc(unsigned int crc, unsigned char input_bit)
{
	unsigned int new_crc;
	unsigned int d = input_bit & 1;
	unsigned int c = crc;

	/* Move queue left.  */
	new_crc = crc << 1;
	/* Mask upper five bits.  */
	new_crc &= 0xF8;
	/* Set lower three bits */
	new_crc |= (d ^ ((c >> 7) & 1));
	new_crc |= (d ^ ((c >> 0) & 1) ^ ((c >> 7) & 1)) << 1;
	new_crc |= (d ^ ((c >> 1) & 1) ^ ((c >> 7) & 1)) << 2;

	return new_crc;
}

static void jp1_write_JTAG(unsigned char packet, unsigned int *p_crc_w)
{
	*p_crc_w = crc_calc(*p_crc_w, packet & 1);
}

unsigned int *jtag_calc_stream_crc(unsigned long stream, int len, unsigned int *p_crc_w)
{
	int i;

	if (len <= 0)
		return (unsigned int *) 0;
	else {
		for (i = 0; i < len; i++)
			jp1_write_JTAG((unsigned char) ((stream >> i) & 1), p_crc_w);
		return p_crc_w;
	}
}
