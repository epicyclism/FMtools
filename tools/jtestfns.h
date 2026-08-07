#pragma once

void CreateJTest16(std::vector<uint16_t>& buf)
{
	int state = 0;
	int cnt = 0;
	for (size_t s = 0; s < buf.size(); ++s)
	{
		switch (state)
		{
		case 0:
		case 1:
		case 2:
		case 3:
			buf[s] = 0xc000;
			++state;
			break;
		case 4:
		case 5:
		case 6:
			buf[s] = 0x4000;
			++state;
			break;
		case 7:
			buf[s] = 0x4000;
			++cnt;
			state = 0;
			if (cnt == 24)
			{
				state = 8;
				cnt = 0;
			}
			break;
		case 8:
		case 9:
		case 10:
		case 11:
			buf[s] = 0xbfff;
			++state;
			break;
		case 12:
		case 13:
		case 14:
			buf[s] = 0x3fff;
			++state;
			break;
		case 15:
			buf[s] = 0x3fff;
			++cnt;
			state = 8;
			if (cnt == 24)
			{
				state = 0;
				cnt = 0;
			}
			break;
		}
	}
}

void CreateJTest24(std::vector<uint32_t>& buf)
{
	int state = 0;
	int cnt = 0;
	for (size_t s = 0; s < buf.size(); ++s)
	{
		switch (state)
		{
		case 0:
		case 1:
		case 2:
		case 3:
			buf[s] = 0xc0000000;
			++state;
			break;
		case 4:
		case 5:
		case 6:
			buf[s] = 0x40000000;
			++state;
			break;
		case 7:
			buf[s] = 0x40000000;
			++cnt;
			state = 0;
			if (cnt == 24)
			{
				state = 8;
				cnt = 0;
			}
			break;
		case 8:
		case 9:
		case 10:
		case 11:
			buf[s] = 0xbfffff00;
			++state;
			break;
		case 12:
		case 13:
		case 14:
			buf[s] = 0x3fffff00;
			++state;
			break;
		case 15:
			buf[s] = 0x3fffff00;
			++cnt;
			state = 8;
			if (cnt == 24)
			{
				state = 0;
				cnt = 0;
			}
			break;
		}
	}
}

void CreateQTest16(std::vector<uint16_t>& buf)
{
	int state = 0;

	for (size_t s = 0; s < buf.size(); ++s)
	{
		switch (state)
		{
		case 0:
		case 1:
		case 2:
		case 3:
			buf[s] = 0xc000;
			++state;
			break;
		case 4:
		case 5:
		case 6:
			buf[s] = 0x4000;
			++state;
			break;
		case 7:
			buf[s] = 0x4000;
			state = 0;
			break;
		}
	}
}

void CreateQTest24(std::vector<uint32_t>& buf)
{
	int state = 0;

	for (size_t s = 0; s < buf.size(); ++s)
	{
		switch (state)
		{
		case 0:
		case 1:
		case 2:
		case 3:
			buf[s] = 0xc0000000;
			++state;
			break;
		case 4:
		case 5:
		case 6:
			buf[s] = 0x40000000;
			++state;
			break;
		case 7:
			buf[s] = 0x40000000;
			state = 0;
			break;
		}
	}
}

void CreateQTestIQ16(std::vector<uint16_t>& buf)
{
	int state = 0;

	for (size_t s = 0; s < buf.size(); ++s)
	{
		switch (state)
		{
		case 0:
		case 1:
		case 2:
			buf[s] = 0xc000;
			++state;
			break;
		case 3:
		case 4:
		case 5:
		case 6:
			buf[s] = 0x4000;
			++state;
			break;
		case 7:
			buf[s] = 0xc000;
			state = 0;
			break;
		}
	}
}

void CreateQTestIQ24(std::vector<uint32_t>& buf)
{
	int state = 0;

	for (size_t s = 0; s < buf.size(); ++s)
	{
		switch (state)
		{
		case 0:
		case 1:
		case 2:
			buf[s] = 0xc0000000;
			++state;
			break;
		case 3:
		case 4:
		case 5:
		case 6:
			buf[s] = 0x40000000;
			++state;
			break;
		case 7:
			buf[s] = 0xc0000000;
			state = 0;
			break;
		}
	}
}

void Create1Bit16(std::vector<uint16_t>& buf, size_t sample_rate)
{
}

void Create1Bit24(std::vector<uint32_t>& buf, size_t sample_rate)
{
}

void CreateJTestF(std::vector<F>& buf)
{
	int state = 0;
	int cnt = 0;
	for (size_t s = 0; s < buf.size(); ++s)
	{
		switch (state)
		{
		case 0:
		case 1:
			buf[s] = F((int32_t)0xc0000000)/0x7fffffff;
			++state;
			break;
		case 2:
			buf[s] =F((int32_t)0x40000000)/ 0x7fffffff;
			++state;
			break;
		case 3:
			buf[s] = F((int32_t)0x40000000)/ 0x7fffffff;
			++cnt;
			state = 0;
			if (cnt == 24)
			{
				state = 4;
				cnt = 0;
			}
			break;
		case 4:
		case 5:
			buf[s] = F((int32_t)0xbfffff00)/ 0x7fffffff;
			++state;
			break;
		case 6:
			buf[s] = F((int32_t)0x3fffff00)/ 0x7fffffff;
			++state;
			break;
		case 7:
			buf[s] = F((int32_t)0x3fffff00)/ 0x7fffffff;
			++cnt;
			state = 4;
			if (cnt == 24)
			{
				state = 0;
				cnt = 0;
			}
			break;
		}
	}
}

void CreateQTestF(std::vector<F>& buf)
{
	int state = 0;

	for (size_t s = 0; s < buf.size(); ++s)
	{
		switch (state)
		{
		case 0:
		case 1:
			buf[s] = F((int32_t)0xc0000000) / 0x7fffffff;
			++state;
			break;
		case 2:
			buf[s] = F((int32_t)0x40000000) / 0x7fffffff;
			++state;
			break;
		case 3:
			buf[s] = F((int32_t)0x40000000) / 0x7fffffff;
			state = 0;
			break;
		}
	}
}

void CreateQTestIQF(std::vector<F>& buf)
{
	int state = 0;

	for (size_t s = 0; s < buf.size(); ++s)
	{
		switch (state)
		{
		case 0:
		case 1:
		case 2:
			buf[s] = (int32_t)0xc0000000;
			++state;
			break;
		case 3:
		case 4:
		case 5:
		case 6:
			buf[s] = (int32_t)0x40000000;
			++state;
			break;
		case 7:
			buf[s] = (int32_t)0xc0000000;
			state = 0;
			break;
		}
	}
}

void Create1BitF(std::vector<F>& buf, size_t sample_rate)
{
}

void CreateKTestF(std::vector<F>& buf, int sublen)
{
	struct KSin
	{
		int state_;
		KSin()
		{
			state_ = 0;
		}
		F operator()()
		{
			F f;
			switch (state_)
			{
			case 0:
				f =  F((int32_t)0xc0000000) / 0x7fffffff;
				++state_;
				break;
			case 1:
				f =  F((int32_t)0xc0000000) / 0x7fffffff;
				++state_;
				break;
			case 2:
				f = F((int32_t)0x40000000) / 0x7fffffff;
				++state_;
				break;
			case 3:
				f = F((int32_t)0x40000000) / 0x7fffffff;
				state_ = 0;
				break;
			}
			return f;
		}
	};
	struct KSquare
	{
		int state_;
		int len_;
		KSquare(int len)
		{
			state_ = 0;
			len_ = len;
		}
		F operator()()
		{
			F f;
			++state_;
			if (state_ < len_ / 2)
			{
				f  = F((int32_t)0x00000100) / 0x7fffffff;
			}
			else
			{
				f  = F(0);
			}
			if (state_ == len_)
				state_ = 0;

			return f;
		}
	};
	KSin ks;
	KSquare kq(sublen);

	for (size_t s = 0; s < buf.size(); ++s)
	{
		buf[s] = ks() - kq();
	}
}
