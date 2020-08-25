//
// Created by Gegel85 on 30/08/2019.
//

#include <TGUI/TGUI.hpp>
#include <SFML/Audio.hpp>
#include <algorithm>
#include "gui.hpp"
#include "../Networking/BgbHandler.hpp"

std::string lastIp;
std::string lastPort;

std::string strToUpper(std::string str)
{
	std::transform(str.begin(), str.end(), str.begin(), ::toupper);
	return str;
}

std::string intToHex(unsigned char i)
{
	std::stringstream stream;
	stream << std::setfill ('0') << std::setw(2) << std::hex << std::uppercase << static_cast<int>(i);
	return stream.str();
}

std::string toLower(std::string str)
{
	std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c){ return std::tolower(c); });
	return str;
}

void makeMainMenuGUI(sf::RenderWindow &window, tgui::Gui &gui, PokemonGen1::GameHandle &game, BattleResources &resources);
void populatePokemonPanel(sf::RenderWindow &window, tgui::Gui &gui, PokemonGen1::GameHandle &game, BattleResources &resources, unsigned index, tgui::Panel::Ptr panel, PokemonGen1::Pokemon &pkmn);

void moveMovePanels(const std::vector<std::pair<unsigned, tgui::ScrollablePanel::Ptr>> &panels)
{
	for (unsigned i = 0; i < panels.size(); i++)
		panels[i].second->setPosition(i % 3 * 250, 10 + i / 3 * 160);
}

void applyMoveFilters(unsigned sorting, std::string query, const std::string &type, std::vector<std::pair<unsigned, tgui::ScrollablePanel::Ptr>> &panels)
{
	std::vector<std::function<bool(const std::pair<unsigned, tgui::ScrollablePanel::Ptr> &p1, const std::pair<unsigned, tgui::ScrollablePanel::Ptr> &p2)>> sortingAlgos = {
		[](const std::pair<unsigned, tgui::ScrollablePanel::Ptr> &p1, const std::pair<unsigned, tgui::ScrollablePanel::Ptr> &p2){
			auto &base1 = PokemonGen1::availableMoves[p1.first];
			auto &base2 = PokemonGen1::availableMoves[p2.first];

			if (base1.getName().substr(0, strlen("Move ")) == base2.getName().substr(0, strlen("Move ")))
				return p1.first < p2.first;
			return base2.getName().substr(0, strlen("Move ")) == "Move " || (
				base1.getName().substr(0, strlen("Move ")) != "Move " &&
				base1.getName() < base2.getName()
			);
		},
		[](const std::pair<unsigned, tgui::ScrollablePanel::Ptr> &p1, const std::pair<unsigned, tgui::ScrollablePanel::Ptr> &p2){
			auto &base1 = PokemonGen1::availableMoves[p1.first];
			auto &base2 = PokemonGen1::availableMoves[p2.first];

			if (base1.getName().substr(0, strlen("Move ")) == base2.getName().substr(0, strlen("Move ")))
				return p1.first < p2.first;
			return base2.getName().substr(0, strlen("Move ")) == "Move " || (
				base1.getName().substr(0, strlen("Move ")) != "Move " &&
				base1.getName() > base2.getName()
			);
		},
		[](const std::pair<unsigned, tgui::ScrollablePanel::Ptr> &p1, const std::pair<unsigned, tgui::ScrollablePanel::Ptr> &p2){
			auto &base1 = PokemonGen1::availableMoves[p1.first];
			auto &base2 = PokemonGen1::availableMoves[p2.first];

			if (base1.getName().substr(0, strlen("Move ")) == base2.getName().substr(0, strlen("Move ")))
				return p1.first < p2.first;
			return base2.getName().substr(0, strlen("Move ")) == "Move " || (
				base1.getName().substr(0, strlen("Move ")) != "Move " &&
				base1.getType() < base2.getType()
			);
		},
		[](const std::pair<unsigned, tgui::ScrollablePanel::Ptr> &p1, const std::pair<unsigned, tgui::ScrollablePanel::Ptr> &p2){
			return p1.first < p2.first;
		},
		[](const std::pair<unsigned, tgui::ScrollablePanel::Ptr> &p1, const std::pair<unsigned, tgui::ScrollablePanel::Ptr> &p2){
			return p1.first > p2.first;
		}
	};

	if (!query.empty()) {
		query = toLower(query);

		panels.erase(std::remove_if(
			panels.begin(),
			panels.end(),
			[&query](const std::pair<unsigned, tgui::ScrollablePanel::Ptr> &p1) {
				return toLower(PokemonGen1::availableMoves[p1.first].getName()).find(query) == std::string::npos;
			}
		), panels.end());
	}
	if (!type.empty())
		panels.erase(std::remove_if(
			panels.begin(),
			panels.end(),
			[&type](const std::pair<unsigned, tgui::ScrollablePanel::Ptr> &p1) {
				return typeToString(PokemonGen1::availableMoves[p1.first].getType()) != type;
			}
		), panels.end());
	std::sort(panels.begin(), panels.end(), sortingAlgos[sorting]);
}

void openChangeMoveBox(tgui::Gui &gui, BattleResources &resources, PokemonGen1::Pokemon &pkmn, unsigned moveIndex, tgui::Button::Ptr moveButton)
{
	auto bigPan = tgui::Panel::create({"100%", "100%"});
	auto panel = tgui::ScrollablePanel::create({"&.w - 20", "&.h - 50"});
	auto basePanel = tgui::ScrollablePanel::create({250, 160});
	auto panels = std::make_shared<std::vector<std::pair<unsigned, tgui::ScrollablePanel::Ptr>>>(PokemonGen1::availableMoves.size());
	auto displayedPanels = std::make_shared<std::vector<std::pair<unsigned, tgui::ScrollablePanel::Ptr>>>(PokemonGen1::availableMoves.size());
	auto filter = tgui::EditBox::create();
	auto typeFilter = tgui::ComboBox::create();
	auto sorting = tgui::ComboBox::create();

	filter->setSize("&.w * 50 / 100 - 30", 20);
	typeFilter->setSize("&.w * 20 / 100 - 20", 20);
	sorting->setSize("&.w * 30 / 100 - 10", 20);

	filter->setDefaultText("Search");
	sorting->addItem("Sort A -> Z");
	sorting->addItem("Sort Z -> A");
	sorting->addItem("Sort by type");
	sorting->addItem("Sort by ascending ID");
	sorting->addItem("Sort by descending ID");
	sorting->setSelectedItemByIndex(0);

	typeFilter->addItem("--Filter by type--", "");
	typeFilter->addItem("Normal", "Normal");
	typeFilter->addItem("Fighting", "Fighting");
	typeFilter->addItem("Fly", "Fly");
	typeFilter->addItem("Poison", "Poison");
	typeFilter->addItem("Ground", "Ground");
	typeFilter->addItem("Rock", "Rock");
	typeFilter->addItem("Bug", "Bug");
	typeFilter->addItem("Ghost", "Ghost");
	typeFilter->addItem("Fire", "Fire");
	typeFilter->addItem("Water", "Water");
	typeFilter->addItem("Grass", "Grass");
	typeFilter->addItem("Electric", "Electric");
	typeFilter->addItem("Psy", "Psy");
	typeFilter->addItem("Ice", "Ice");
	typeFilter->addItem("Dragon", "Dragon");
	typeFilter->addItem("???", "Unknown");
	typeFilter->setSelectedItemByIndex(0);

	auto refresh = [displayedPanels, panels, filter, sorting, typeFilter]{
		for (auto &panel : *panels)
			panel.second->setPosition(-200, -200);
		*displayedPanels = *panels;
		applyMoveFilters(sorting->getSelectedItemIndex(), filter->getText(), typeFilter->getSelectedItemId(), *displayedPanels);
		moveMovePanels(*displayedPanels);
	};

	filter->onTextChange.connect(refresh);
	sorting->onItemSelect.connect(refresh);
	typeFilter->onItemSelect.connect(refresh);

	filter->setPosition(10, 10);
	sorting->setPosition("&.w * 70 / 100", 10);
	typeFilter->setPosition("&.w * 50 / 100", 10);
	panel->setPosition(10, 40);

	bigPan->add(typeFilter);
	bigPan->add(sorting);
	bigPan->add(filter);

	basePanel->loadWidgetsFromFile("assets/movePanel.gui");

	for (unsigned i = 0; i < panels->size(); i++) {
		auto &pan = (panels->operator[](i) = {i, tgui::ScrollablePanel::copy(basePanel)}).second;
		auto &move = PokemonGen1::availableMoves[i];
		auto type = tgui::Picture::create(resources.types[typeToString(move.getType())]);
		auto category = tgui::Picture::create(resources.categories[move.getCategory()]);
		auto pps = pan->get<tgui::EditBox>("PPs");
		auto pow = pan->get<tgui::EditBox>("Power");
		auto acc = pan->get<tgui::EditBox>("Accuracy");
		auto name = pan->get<tgui::Button>("Name");
		auto effects = pan->get<tgui::TextBox>("AdditionalEffects");

		name->setText(strToUpper(move.getName()));
		name->onClick.connect([&move, moveButton, bigPan, &gui, moveIndex, &pkmn]{
			pkmn.setMove(moveIndex, move);
			gui.remove(bigPan);
			moveButton->setText(move.getName());
		});
		pps->setText(std::to_string(move.getMaxPP()));
		pow->setText(move.getPower() ? std::to_string(move.getPower()) : "-");
		acc->setText(move.getAccuracy() > 100 ? "-" : std::to_string(move.getAccuracy()));
		effects->setVisible(!move.getDescription().empty());
		effects->setText(move.getDescription());

		type->setPosition(190, 2);
		category->setPosition(206, 29);
		pan->add(type);
		pan->add(category);

		panel->add(pan);
	}
	*displayedPanels = *panels;
	applyMoveFilters(0, "", "", *displayedPanels);
	moveMovePanels(*displayedPanels);
	bigPan->add(panel);
	gui.add(bigPan);
}

void movePkmnsPanels(const std::vector<std::pair<unsigned, tgui::ScrollablePanel::Ptr>> &panels)
{
	for (unsigned i = 0; i < panels.size(); i++)
		panels[i].second->setPosition(28 + i % 3 * 248, 10 + i / 3 * 190);
}

void applyPkmnsFilters(unsigned sorting, std::string query, const std::string &type, std::vector<std::pair<unsigned, tgui::ScrollablePanel::Ptr>> &panels)
{
	std::vector<std::function<bool(const std::pair<unsigned, tgui::ScrollablePanel::Ptr> &p1, const std::pair<unsigned, tgui::ScrollablePanel::Ptr> &p2)>> sortingAlgos = {
		[](const std::pair<unsigned, tgui::ScrollablePanel::Ptr> &p1, const std::pair<unsigned, tgui::ScrollablePanel::Ptr> &p2){
			auto &base1 = PokemonGen1::pokemonList[p1.first];
			auto &base2 = PokemonGen1::pokemonList[p2.first];

			if (base1.name == base2.name)
				return p1.first < p2.first;
			return base2.name == "MISSINGNO." || (
				base1.name != "MISSINGNO." &&
				base1.name < base2.name
			);
		},
		[](const std::pair<unsigned, tgui::ScrollablePanel::Ptr> &p1, const std::pair<unsigned, tgui::ScrollablePanel::Ptr> &p2){
			auto &base1 = PokemonGen1::pokemonList[p1.first];
			auto &base2 = PokemonGen1::pokemonList[p2.first];

			if (base1.name == base2.name)
				return p1.first < p2.first;
			return base2.name == "MISSINGNO." || (
				base1.name != "MISSINGNO." &&
				base1.name > base2.name
			);
		},
		[](const std::pair<unsigned, tgui::ScrollablePanel::Ptr> &p1, const std::pair<unsigned, tgui::ScrollablePanel::Ptr> &p2){
			return p1.first < p2.first;
		},
		[](const std::pair<unsigned, tgui::ScrollablePanel::Ptr> &p1, const std::pair<unsigned, tgui::ScrollablePanel::Ptr> &p2){
			return p1.first > p2.first;
		}
	};

	if (!query.empty()) {
		query = toLower(query);

		panels.erase(std::remove_if(
			panels.begin(),
			panels.end(),
			[&query](const std::pair<unsigned, tgui::ScrollablePanel::Ptr> &p1) {
				return toLower(PokemonGen1::pokemonList[p1.first].name).find(query) == std::string::npos;
			}
		), panels.end());
	}
	if (!type.empty())
		panels.erase(std::remove_if(
			panels.begin(),
			panels.end(),
			[&type](const std::pair<unsigned, tgui::ScrollablePanel::Ptr> &p1) {
				auto &base = PokemonGen1::pokemonList[p1.first];

				return typeToString(base.typeA) != type && typeToString(base.typeB) != type;
			}
		), panels.end());
	std::sort(panels.begin(), panels.end(), sortingAlgos[sorting]);
}

void openChangePkmnBox(tgui::Gui &gui, PokemonGen1::GameHandle &game, BattleResources &resources, unsigned index, PokemonGen1::Pokemon &pkmn, sf::RenderWindow &window, tgui::Panel::Ptr pkmnPan)
{
	auto bigPan = tgui::Panel::create({"100%", "100%"});
	auto panel = tgui::ScrollablePanel::create({"&.w - 20", "&.h - 50"});
	auto basePanel = tgui::ScrollablePanel::create({220, 170});
	auto panels = std::make_shared<std::vector<std::pair<unsigned, tgui::ScrollablePanel::Ptr>>>(PokemonGen1::pokemonList.size());
	auto displayedPanels = std::make_shared<std::vector<std::pair<unsigned, tgui::ScrollablePanel::Ptr>>>(PokemonGen1::pokemonList.size());
	auto filter = tgui::EditBox::create();
	auto sorting = tgui::ComboBox::create();
	auto typeFilter = tgui::ComboBox::create();

	filter->setSize("&.w * 50 / 100 - 30", 20);
	typeFilter->setSize("&.w * 20 / 100 - 20", 20);
	sorting->setSize("&.w * 30 / 100 - 10", 20);

	filter->setDefaultText("Search");
	sorting->addItem("Sort A -> Z");
	sorting->addItem("Sort Z -> A");
	sorting->addItem("Sort by ascending ID");
	sorting->addItem("Sort by descending ID");
	sorting->setSelectedItemByIndex(0);

	typeFilter->addItem("--Filter by type--", "");
	typeFilter->addItem("Normal", "Normal");
	typeFilter->addItem("Fighting", "Fighting");
	typeFilter->addItem("Fly", "Fly");
	typeFilter->addItem("Poison", "Poison");
	typeFilter->addItem("Ground", "Ground");
	typeFilter->addItem("Rock", "Rock");
	typeFilter->addItem("Bug", "Bug");
	typeFilter->addItem("Ghost", "Ghost");
	typeFilter->addItem("Fire", "Fire");
	typeFilter->addItem("Water", "Water");
	typeFilter->addItem("Grass", "Grass");
	typeFilter->addItem("Electric", "Electric");
	typeFilter->addItem("Psy", "Psy");
	typeFilter->addItem("Ice", "Ice");
	typeFilter->addItem("Dragon", "Dragon");
	typeFilter->addItem("???", "???");
	typeFilter->setSelectedItemByIndex(0);

	auto refresh = [displayedPanels, panels, filter, sorting, typeFilter]{
		for (auto &panel : *panels)
			panel.second->setPosition(-200, -200);
		*displayedPanels = *panels;
		applyPkmnsFilters(sorting->getSelectedItemIndex(), filter->getText(), typeFilter->getSelectedItemId(), *displayedPanels);
		movePkmnsPanels(*displayedPanels);
	};

	filter->onTextChange.connect(refresh);
	sorting->onItemSelect.connect(refresh);
	typeFilter->onItemSelect.connect(refresh);

	filter->setPosition(10, 10);
	sorting->setPosition("&.w * 70 / 100", 10);
	typeFilter->setPosition("&.w * 50 / 100", 10);
	panel->setPosition(10, 40);

	bigPan->add(typeFilter);
	bigPan->add(sorting);
	bigPan->add(filter);

	basePanel->loadWidgetsFromFile("assets/pkmnPreview.gui");

	for (unsigned i = 0; i < panels->size(); i++) {
		auto &pan = (panels->operator[](i) = {i, tgui::ScrollablePanel::copy(basePanel)}).second;
		auto &base = PokemonGen1::pokemonList[i];
		auto type1 = tgui::Picture::create(resources.types[typeToString(base.typeA)]);
		auto type2 = tgui::Picture::create(resources.types[typeToString(base.typeB)]);
		auto sprite = pan->get<tgui::BitmapButton>("Species");
		auto hp = pan->get<tgui::TextBox>("HP");
		auto atk = pan->get<tgui::TextBox>("ATK");
		auto def = pan->get<tgui::TextBox>("DEF");
		auto spd = pan->get<tgui::TextBox>("SPD");
		auto spe = pan->get<tgui::TextBox>("SPE");
		auto name = pan->get<tgui::TextBox>("SpeciesName");
		auto &stats = base.statsAtLevel[pkmn.getLevel()];

		name->setText(strToUpper(base.name));
		hp->setText(std::to_string(stats.HP));
		atk->setText(std::to_string(stats.ATK));
		def->setText(std::to_string(stats.DEF));
		spd->setText(std::to_string(stats.SPD));
		spe->setText(std::to_string(stats.SPE));
		sprite->setImage(resources.pokemonsFront[i]);

		sprite->onClick.connect([&window, pkmnPan, &resources, bigPan, &gui, index, &pkmn, &game, &base]{
			game.changePokemon(index, pkmn.getNickname(), pkmn.getLevel(), base, pkmn.getMoveSet());
			gui.remove(bigPan);
			populatePokemonPanel(window, gui, game, resources, index, pkmnPan, game.getPokemon(index));
		});

		type1->setPosition(115, 152);
		type2->setPosition(170, 152);
		type2->setVisible(base.typeA != base.typeB);
		pan->add(type1);
		pan->add(type2);

		panel->add(pan);
	}
	*displayedPanels = *panels;
	applyPkmnsFilters(0, "", "", *displayedPanels);
	movePkmnsPanels(*displayedPanels);
	bigPan->add(panel);
	gui.add(bigPan);
}

void populatePokemonPanel(sf::RenderWindow &window, tgui::Gui &gui, PokemonGen1::GameHandle &game, BattleResources &resources, unsigned index, tgui::Panel::Ptr panel, PokemonGen1::Pokemon &pkmn)
{
	panel->loadWidgetsFromFile("assets/pkmnPanel.gui");

	auto type1 = tgui::Picture::create(resources.types[typeToString(pkmn.getTypes().first)]);
	auto type2 = tgui::Picture::create(resources.types[typeToString(pkmn.getTypes().second)]);
	auto sprite = panel->get<tgui::BitmapButton>("Species");
	auto hp = panel->get<tgui::TextBox>("HP");
	auto atk = panel->get<tgui::TextBox>("ATK");
	auto def = panel->get<tgui::TextBox>("DEF");
	auto spd = panel->get<tgui::TextBox>("SPD");
	auto spe = panel->get<tgui::TextBox>("SPE");
	std::vector<tgui::Button::Ptr> moves = {
		panel->get<tgui::Button>("Move1"),
		panel->get<tgui::Button>("Move2"),
		panel->get<tgui::Button>("Move3"),
		panel->get<tgui::Button>("Move4")
	};
	auto name = panel->get<tgui::TextBox>("SpeciesName");
	auto nick = panel->get<tgui::EditBox>("Nickname");
	auto level = panel->get<tgui::EditBox>("Level");
	auto remove = panel->get<tgui::Button>("Remove");
	auto &moveSet = pkmn.getMoveSet();

	for (size_t i = 0; i < moveSet.size(); i++) {
		moves[i]->setText(moveSet[i].getName());
		moves[i]->onClick.connect([i, &gui, &pkmn, moves, &resources]{
			openChangeMoveBox(gui,  resources, pkmn, i, moves[i]);
		});
	}
	level->setText(std::to_string(pkmn.getLevel()));
	name->setText(strToUpper(PokemonGen1::pokemonList[pkmn.getID()].name));
	nick->setText(pkmn.getNickname());
	hp->setText(std::to_string(pkmn.getBaseStats().HP));
	atk->setText(std::to_string(pkmn.getBaseStats().ATK));
	def->setText(std::to_string(pkmn.getBaseStats().DEF));
	spd->setText(std::to_string(pkmn.getBaseStats().SPD));
	spe->setText(std::to_string(pkmn.getBaseStats().SPE));
	sprite->setImage(resources.pokemonsFront[pkmn.getID()]);

	nick->onTextChange.connect([&pkmn, nick]{
		pkmn.setNickname(nick->getText());
	});
	level->onTextChange.connect([&pkmn, level, hp, atk, def, spd, spe]{
		if (level->getText().isEmpty())
			return;

		auto newLevel = std::stoul(level->getText().toAnsiString());

		if (newLevel > 255) {
			newLevel = 255;
			level->setText("255");
		}

		pkmn.setLevel(newLevel);
		hp->setText(std::to_string(pkmn.getBaseStats().HP));
		atk->setText(std::to_string(pkmn.getBaseStats().ATK));
		def->setText(std::to_string(pkmn.getBaseStats().DEF));
		spd->setText(std::to_string(pkmn.getBaseStats().SPD));
		spe->setText(std::to_string(pkmn.getBaseStats().SPE));
	});
	remove->onClick.connect([index, &window, &gui, &game, &resources]{
		game.deletePokemon(index);
		makeMainMenuGUI(window, gui, game, resources);
	});
	sprite->onClick.connect([&gui, &game, &resources, index, &pkmn, &window, panel]{
		openChangePkmnBox(gui, game, resources, index, pkmn, window, panel);
	});

	type1->setPosition(5, 100);
	type1->setSize(44, 16);
	type2->setPosition(52, 100);
	type2->setSize(44, 16);
	type2->setVisible(pkmn.getTypes().first != pkmn.getTypes().second);
	panel->add(type1);
	panel->add(type2);
}

void makeMainMenuGUI(sf::RenderWindow &window, tgui::Gui &gui, PokemonGen1::GameHandle &game, BattleResources &resources)
{
	gui.loadWidgetsFromFile("assets/mainMenu.gui");

	auto connect = gui.get<tgui::Button>("Connect");
	auto ip = gui.get<tgui::EditBox>("IP");
	auto port = gui.get<tgui::EditBox>("Port");
	auto error = gui.get<tgui::TextBox>("Error");
	auto team = gui.get<tgui::Panel>("Team");
	auto ready = gui.get<tgui::Button>("Ready");
	auto name = gui.get<tgui::EditBox>("Name");
	auto teamPanel = gui.get<tgui::Panel>("Team");
	std::vector<tgui::Panel::Ptr> panels{
		teamPanel->get<tgui::Panel>("Pkmn1"),
		teamPanel->get<tgui::Panel>("Pkmn2"),
		teamPanel->get<tgui::Panel>("Pkmn3"),
		teamPanel->get<tgui::Panel>("Pkmn4"),
		teamPanel->get<tgui::Panel>("Pkmn5"),
		teamPanel->get<tgui::Panel>("Pkmn6"),
	};

	ip->setText(lastIp);
	ip->onTextChange.connect([ip]{
		lastIp = ip->getText();
	});
	port->setText(lastPort);
	port->onTextChange.connect([port]{
		lastPort = port->getText();
	});
	ready->setText(game.isReady() ? "You are ready" : "You are not ready");
	ready->onClick.connect([&game, ready, panels, name] {
		if (game.getStage() >= PokemonGen1::EXCHANGE_POKEMONS)
			return;

		game.setReady(!game.isReady());
		ready->setText(game.isReady() ? "You are ready" : "You are not ready");
		name->setEnabled(!game.isReady());
		for (auto &panel : panels)
			for (auto &widget : panel->getWidgets())
				widget->setEnabled(!game.isReady());
	});
	name->setText(game.getTrainerName());
	connect->setText(!game.isConnected() ? "Connect" : "Disconnect");
	connect->onClick.connect([&game, connect, error, port, ip]{
		error->setText("");
		if (game.isConnected()) {
			game.disconnect();
			connect->setText("Connect");
			port->setEnabled(true);
			ip->setEnabled(true);
		} else {
			try {
				auto p = std::stoul(port->getText().toAnsiString());

				if (p > 65535)
					throw std::out_of_range("");
				game.connect(ip->getText(), p);
				port->setEnabled(false);
				ip->setEnabled(false);
			} catch (std::invalid_argument &) {
				error->setText("The port is not a valid number");
				return;
			} catch (std::out_of_range &) {
				error->setText("The port is not in range 0-65535");
				return;
			} catch (std::exception &e) {
				error->setText(e.what());
				return;
			}
			connect->setText("Disconnect");
		}
	});
	name->onTextChange.connect([&window, &game, name]{
		game.setTrainerName(name->getText());
		window.setTitle(game.getTrainerName() + " - Preparing battle");
	});

	for (auto &pkmnPan : panels)
		pkmnPan->removeAllWidgets();
	for (unsigned i = 0; i < game.getPokemonTeam().size(); i++)
		populatePokemonPanel(window, gui, game, resources, i, panels[i], game.getPokemon(i));
	for (unsigned i = game.getPokemonTeam().size(); i < 6; i++) {
		tgui::Button::Ptr but = tgui::Button::create("+");

		but->setPosition(10, 10);
		but->setSize({"&.w - 20", "&.h - 20"});
		but->onClick.connect([&window, &gui, &game, &resources]{
			auto size = game.getPokemonTeam().size();

			game.setTeamSize(size + 1);
			game.getPokemon(size).setLevel(100);
			makeMainMenuGUI(window, gui, game, resources);
		});
		panels[i]->add(but);
	}
}

void mainMenu(sf::RenderWindow &window, PokemonGen1::GameHandle &game, BattleResources &resources)
{
	tgui::Gui gui{window};

	window.setSize({800, 640});
	makeMainMenuGUI(window, gui, game, resources);

	game.setReady(false);
	window.setTitle(game.getTrainerName() + " - Preparing battle");
	while (window.isOpen() && game.getStage() != PokemonGen1::BATTLE) {
		sf::Event event;

		while (window.pollEvent(event)) {
			if (event.type == sf::Event::Closed)
				window.close();
			gui.handleEvent(event);
		}

		window.clear();
		gui.draw();
		window.display();

		auto status = gui.get<tgui::TextBox>("Status");

		if (!status)
			return;

		if (game.isConnected())
			switch (game.getStage()) {
			case PokemonGen1::PKMN_CENTER:
				status->setText("Opponent not ready");
				break;
			case PokemonGen1::PINGING_OPPONENT:
				status->setText("Waiting for opponent to save the game");
				break;
			case PokemonGen1::ROOM_CHOOSE:
				status->setText("Choosing colosseum");
				break;
			case PokemonGen1::PING_POKEMON_EXCHANGE:
				status->setText(game.isReady() ? "Waiting for opponent to start the game" : "Waiting for you to be ready");
				break;
			case PokemonGen1::EXCHANGE_POKEMONS:
				status->setText("Exchanging battle data");
				break;
			case PokemonGen1::BATTLE:
				status->setText("In battle");
				break;
			}
		else
			status->setText("Disconnected");
	}
}

void loadResources(BattleResources &resources)
{
	resources.start.openFromFile("assets/sounds/battle_intro.wav");
	resources.loop.openFromFile("assets/sounds/battle_loop.wav");
	resources.levelSprite.loadFromFile("assets/level_icon.png");

	resources.categories[0].loadFromFile("assets/move_categories/physical.png");
	resources.categories[1].loadFromFile("assets/move_categories/special.png");
	resources.categories[2].loadFromFile("assets/move_categories/status.png");

	for (int i = 0; i < 256; i++)
		if (!resources.pokemonsBack[i].loadFromFile("assets/back_sprites/" + std::to_string(i) + "_back.png"))
			resources.pokemonsBack[i].loadFromFile("assets/back_sprites/missingno_back.png");

	for (int i = 0; i < 256; i++)
		if (!resources.pokemonsFront[i].loadFromFile("assets/front_sprites/" + std::to_string(i) + "_front.png"))
			resources.pokemonsFront[i].loadFromFile("assets/front_sprites/missingno_front.png");

	resources.balls[0].loadFromFile("assets/pokeballs/pkmnOK.png");
	resources.balls[1].loadFromFile("assets/pokeballs/pkmnNO.png");
	resources.balls[2].loadFromFile("assets/pokeballs/pkmnFNT.png");
	resources.balls[3].loadFromFile("assets/pokeballs/pkmnSTATUS.png");

	resources.font.loadFromFile("assets/font.ttf");
	resources.hitSounds[0].loadFromFile("assets/sounds/not_effective_hit_sound.wav");
	resources.hitSounds[1].loadFromFile("assets/sounds/hit_sound.wav");
	resources.hitSounds[2].loadFromFile("assets/sounds/very_effective_hit_sound.wav");

	for (int i = 0; i <= TYPE_DRAGON; i++)
		try {
			resources.types.at(typeToString(static_cast<PokemonTypes>(i)));
		} catch (std::out_of_range &) {
			resources.types[typeToString(static_cast<PokemonTypes>(i))].loadFromFile("assets/types/type_" + toLower(typeToString(static_cast<PokemonTypes>(i))) + ".png");
		}

	resources.trainer[0][0].loadFromFile("assets/back_sprites/trainer_shadow_back.png");
	resources.trainer[0][1].loadFromFile("assets/front_sprites/trainer_shadow_front.png");
	resources.trainer[1][0].loadFromFile("assets/back_sprites/trainer_back.png");
	resources.trainer[1][1].loadFromFile("assets/front_sprites/trainer_front.png");

	resources.trainerLand.loadFromFile("assets/sounds/trainer_land.wav");
	resources.hpOverlay.loadFromFile("assets/hp_overlay.png");
	resources.choicesHUD.loadFromFile("assets/choices.png");
	resources.attackHUD.loadFromFile("assets/attacks_overlay.png");
	resources.waitingHUD.loadFromFile("assets/wait_overlay.png");

	resources.arrows[0].loadFromFile("assets/arrow.png");
	resources.arrows[1].loadFromFile("assets/selectArrow.png");

	resources.boxes[0].loadFromFile("assets/text_box.png");
	resources.boxes[1].loadFromFile("assets/VS_box.png");
	resources.boxes[2].loadFromFile("assets/pkmns_border.png");
	resources.boxes[3].loadFromFile("assets/pkmns_border_player_side.png");

	for (int i = 0; i < 256; i++)
		resources.battleCries[i].loadFromFile("assets/cries/" + std::to_string(i) + "_cry.wav");
}

void gui(const std::string &trainerName, bool ai)
{
	std::vector<std::string> battleLog;
	PokemonGen1::BattleAction nextAction = PokemonGen1::NoAction;
	PokemonGen1::GameHandle handler(
		[](const ByteHandle &byteHandle, const LoopHandle &loopHandler, const std::string &ip, unsigned short port)
		{
			return new BGBHandler(byteHandle, byteHandle, loopHandler, ip, port, getenv("MAX_DEBUG"));
		},
		[&nextAction](PokemonGen1::GameHandle &handle) {
			PokemonGen1::BattleAction action = nextAction;
			const PokemonGen1::BattleState &state = handle.getBattleState();

			if (!state.opponentTeam[state.opponentPokemonOnField].getHealth())
				return static_cast<PokemonGen1::BattleAction>(PokemonGen1::Switch1 + state.pokemonOnField);

			if (action)
				nextAction = PokemonGen1::NoAction;
			return action;
		},
		trainerName,
		[&battleLog](const std::string &msg){ battleLog.push_back(msg); },
		false,
		getenv("MIN_DEBUG")
	);
	sf::RenderWindow window{{800, 640}, trainerName};
	BattleResources resources;

	loadResources(resources);
	handler.setTeamSize(6);
	for (int i = 0; i < 6; i++)
		handler.changePokemon(
			i,
			handler.getPokemonTeam()[i].getNickname(),
			100,
			PokemonGen1::pokemonList.at(handler.getPokemonTeam()[i].getID()),
			std::vector<PokemonGen1::Move>(handler.getPokemonTeam()[i].getMoveSet())
		);
	window.setFramerateLimit(60);
	handler.setReady(false);
	while (window.isOpen()) {
		if (handler.getStage() != PokemonGen1::BATTLE)
			mainMenu(window, handler, resources);
		else
			battle(window, handler, resources, battleLog, nextAction);
	}
}