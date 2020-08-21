//
// Created by Gegel85 on 15/07/2019.
//

#include "PokemonRandomGenerator.hpp"
#include "GameHandle.hpp"

namespace PokemonGen1
{
	PokemonRandomGenerator::PokemonRandomGenerator() :
		_random(time(nullptr))
	{
	}

	void PokemonRandomGenerator::makeRandomList(unsigned int size)
	{
		std::uniform_int_distribution distribution{0, SYNC_BYTE - 1};

		this->_currentIndex = 0;
		this->_numbers.clear();
		while (size--)
			this->_numbers.push_back(distribution(this->_random));
		if (!this->_numbers.empty() && !this->_numbers[0])
			this->_numbers[0] = 1;
	}

	const std::vector<unsigned char>& PokemonRandomGenerator::getList()
	{
		return this->_numbers;
	}

	void PokemonRandomGenerator::setList(const std::vector<unsigned char> &list)
	{
		this->_numbers = list;
		this->_currentIndex = 0;
		if (!this->_numbers.empty() && !this->_numbers[0])
			this->_numbers[0] = 1;
	}

	unsigned char PokemonRandomGenerator::operator()()
	{
		unsigned char value = this->_numbers[this->_currentIndex];

		this->_currentIndex++;
		if (this->_numbers.size() == this->_currentIndex) {
			this->_currentIndex = 0;
			for (auto &elem : this->_numbers)
				elem = elem * 5 + 1;
		}
		return value;
	}
}