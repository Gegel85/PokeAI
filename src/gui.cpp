//
// Created by Gegel85 on 30/08/2019.
//

#include <TGUI/TGUI.hpp>
#include <utility>
#include "gui.hpp"
#include "GameHandle.hpp"
#include "BgbHandler.hpp"

std::string intToHex(unsigned char i)
{
	std::stringstream stream;
	stream << std::setfill ('0') << std::setw(2) << std::hex << std::uppercase << static_cast<int>(i);
	return stream.str();
}

tgui::Button::Ptr makeButton(const std::string &content, tgui::Layout x, tgui::Layout y, const std::function<void (tgui::Button::Ptr button)> &handler, int width = 0, int height = 0)
{
	tgui::Button::Ptr button = tgui::Button::create(content);

	button->setPosition(std::move(x), std::move(y));
	if (handler)
		button->connect("pressed", handler, button);
	if (height && width)
		button->setSize(width, height);
	return button;
}

tgui::TextBox::Ptr makeTextBox(tgui::Layout x, tgui::Layout y, tgui::Layout width, tgui::Layout height, const std::string &placeholder = "")
{
	tgui::TextBox::Ptr box = tgui::TextBox::create();

	box->setPosition(std::move(x), std::move(y));
	box->setSize({width, height});
	box->setText(placeholder);
	box->setReadOnly(true);
	box->setEnabled(false);
	return box;
}

tgui::TextBox::Ptr makeTypeBox(tgui::Layout x, tgui::Layout y, tgui::Layout width, tgui::Layout height, const std::string &placeholder = "")
{
	tgui::TextBox::Ptr box = tgui::TextBox::create();

	box->setPosition(std::move(x), std::move(y));
	box->setSize({std::move(width), std::move(height)});
	box->setText(placeholder);
	box->setReadOnly(false);
	return box;
}

tgui::Picture::Ptr makePicture(const std::string &path, tgui::Layout x, tgui::Layout y, tgui::Layout width, tgui::Layout height)
{
	tgui::Picture::Ptr pic = tgui::Picture::create(path);

	pic->setPosition(std::move(x), std::move(y));
	pic->setSize({std::move(width), std::move(height)});
	pic->ignoreMouseEvents();
	return pic;
}

std::string toLower(std::string str)
{
	std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c){ return std::tolower(c); });
	return str;
}

void updatePokemonTeam(tgui::Gui &gui, PokemonGen1::GameHandle &game)
{
	for (unsigned i = 0; i < game.getPokemonTeam().size(); i++) {
		auto &pkmn = game.getPokemonTeam()[i];

		gui.get("pkmnFrontButton" + std::to_string(i))->setVisible(true);
		gui.remove(gui.get("pkmnFront" + std::to_string(i)));
		gui.add(makePicture("assets/front_sprites/" + std::to_string(pkmn.getID()) + "_front.png", 300 + (i % 3) * 150, 50 + (i / 3) * 300, 96, 96), "pkmnFront" + std::to_string(i));
		gui.remove(gui.get("type1" + std::to_string(i)));
		gui.add(makePicture("assets/types/type_" + toLower(typeToString(pkmn.getTypes().first)) + ".png", 300 + (i % 3) * 150, 150 + (i / 3) * 300, 48, 16), "type1" + std::to_string(i));
		if (pkmn.getTypes().first != pkmn.getTypes().second) {
			gui.remove(gui.get("type2" + std::to_string(i)));
			gui.add(makePicture("assets/types/type_" + toLower(typeToString(pkmn.getTypes().second)) + ".png", 348 + (i % 3) * 150, 150 + (i / 3) * 300, 48, 16), "type2" + std::to_string(i));
		} else
			gui.get("type2" + std::to_string(i))->setVisible(false);
		gui.get("name" + std::to_string(i))->setVisible(true);
		gui.get<tgui::TextBox>("name" + std::to_string(i))->setText(intToHex(pkmn.getID()) + " " + pkmn.getSpeciesName());
		gui.get("nickname" + std::to_string(i))->setVisible(true);
		gui.get<tgui::TextBox>("nickname" + std::to_string(i))->setText(pkmn.getNickname());
		for (int j = 0; j < 4; j++) {
			gui.get("move" + std::to_string(j) + std::to_string(i))->setVisible(true);
			gui.get<tgui::Button>("move" + std::to_string(j) + std::to_string(i))->setText(game.getPokemonTeam()[i].getMoveSet()[j].getName());
		}
	}

	for (unsigned i = game.getPokemonTeam().size(); i < 6; i++) {
		gui.get("pkmnFrontButton" + std::to_string(i))->setVisible(false);
		gui.get("pkmnFront" + std::to_string(i))->setVisible(false);
		gui.get("type1" + std::to_string(i))->setVisible(false);
		gui.get("type2" + std::to_string(i))->setVisible(false);
		gui.get("name" + std::to_string(i))->setVisible(false);
		gui.get("nickname" + std::to_string(i))->setVisible(false);
		for (unsigned j = 0; j < 4; j++)
			gui.get("move" + std::to_string(j) + std::to_string(i))->setVisible(false);
	}
}

void makeMainMenuGUI(tgui::Gui &gui, tgui::Gui &selectPkmnMenu, tgui::Gui &selectMovePanel, PokemonGen1::GameHandle &game, unsigned &id, unsigned &menu)
{
	gui.removeAllWidgets();
	gui.add(makeTypeBox(10, 10, 200, 25, "address"), "ip");
	gui.add(makeTypeBox(10, 40, 200, 25, "port"), "port");
	gui.add(makeButton("Connect", 10, 70, [&game, &gui](tgui::Button::Ptr button){
		if (game.isConnected()) {
			gui.get<tgui::TextBox>("ip")->setEnabled(true);
			gui.get<tgui::TextBox>("port")->setEnabled(true);
			try {
				game.disconnect();
				gui.get<tgui::TextBox>("errorBox")->setText("Disconnected");
			} catch (std::exception &e) {
				gui.get<tgui::TextBox>("errorBox")->setText(e.what());
			}
			button->setText("Connect");
			return;
		}
		try {
			unsigned long port = std::stoul(gui.get<tgui::TextBox>("port")->getText().toAnsiString());

			if (port > 65535)
				throw std::out_of_range("out_of_range");

			game.connect(gui.get<tgui::TextBox>("ip")->getText(), port);
			button->setText("Disconnect");
			gui.get<tgui::TextBox>("ip")->setEnabled(false);
			gui.get<tgui::TextBox>("port")->setEnabled(false);
			gui.get<tgui::TextBox>("errorBox")->setText("Connected to " + gui.get<tgui::TextBox>("ip")->getText());
		} catch (std::out_of_range &) {
			gui.get<tgui::TextBox>("errorBox")->setText("Error: invalid port number");
		} catch (std::invalid_argument &) {
			gui.get<tgui::TextBox>("errorBox")->setText("Error: invalid port number");
		} catch (std::exception &e) {
			gui.get<tgui::TextBox>("errorBox")->setText(e.what());
		}
	}), "connect");
	gui.add(makeTextBox(10, 100, 200, 100), "errorBox");
	gui.add(makeButton("You are not ready", 10, 550, [&game](tgui::Button::Ptr){
		game.setReady(!game.isReady());
	}), "ready");
	gui.add(makeTextBox(10, 580, 200, 50, "Disconnected"), "status");

	tgui::Slider::Ptr slider = tgui::Slider::create();
	slider->setPosition(300, 10);
	slider->setSize(200, 18);
	slider->setValue(6);
	slider->setMaximum(6);
	slider->setMinimum(1);
	slider->connect("ValueChanged", [&game, &gui](tgui::Slider::Ptr slider) {
		slider->setValue(static_cast<int>(slider->getValue()));
		if (slider->getValue() != game.getPokemonTeam().size()) {
			game.setTeamSize(slider->getValue());
			updatePokemonTeam(gui, game);
		}
	}, slider);
	gui.add(slider);

	for (int i = 0; i < 6; i++) {
		gui.add(makeButton("", 300 + (i % 3) * 150, 50 + (i / 3) * 300, [&menu, &gui, &id, &selectPkmnMenu](tgui::Button::Ptr button){
			id = std::stol(gui.getWidgetName(button).substr(15));
			menu = 1;
		}, 96, 96), "pkmnFrontButton" + std::to_string(i));
		gui.add(makePicture("assets/front_sprites/1_front.png", 300 + (i % 3) * 150, 50 + (i / 3) * 300, 96, 96), "pkmnFront" + std::to_string(i));
		gui.add(makePicture("assets/types/type_ground.png", 300 + (i % 3) * 150, 150 + (i / 3) * 300, 48, 16), "type1" + std::to_string(i));
		gui.add(makePicture("assets/types/type_rock.png", 348 + (i % 3) * 150, 150 + (i / 3) * 300, 48, 16), "type2" + std::to_string(i));
		gui.add(makeTextBox(300 + (i % 3) * 150, 170 + (i / 3) * 300, 96, 20, "01 RHYDON"), "name" + std::to_string(i));
		gui.add(makeTypeBox(300 + (i % 3) * 150, 190 + (i / 3) * 300, 96, 20, ""), "nickname" + std::to_string(i));
		for (int j = 0; j < 4; j++)
			gui.add(makeButton(game.getPokemonTeam()[i].getMoveSet()[j].getName(), 300 + (i % 3) * 150, 210 + (i / 3) * 300 + 20 * j, [&id, &gui, &menu](tgui::Button::Ptr button) {
				id = std::stol(gui.getWidgetName(button).substr(4));
				menu = 2;
			}, 96, 20), "move" + std::to_string(j) + std::to_string(i));
		gui.get("nickname" + std::to_string(i))->connect("TextChanged", [&gui, &game](tgui::TextBox::Ptr box){
			if (box->getText().getSize() > 10) {
				box->setText(box->getText().substring(0, 10));
				return;
			}

			unsigned id = std::stol(gui.getWidgetName(box).substr(8));

			game.changePokemon(
				id,
				box->getText(),
				game.getPokemonTeam()[id].getLevel(),
				PokemonGen1::pokemonList.at(game.getPokemonTeam()[id].getID()),
				std::vector<PokemonGen1::Move>(game.getPokemonTeam()[id].getMoveSet())
			);
		}, gui.get<tgui::TextBox>("nickname" + std::to_string(i)));
	}


	//Pokemon select
	tgui::Scrollbar::Ptr scrollbar = tgui::Scrollbar::create();
	scrollbar->setPosition(780, 10);
	scrollbar->setSize(18, 620);
	scrollbar->setMaximum(6384);
	scrollbar->setViewportSize(640);
	scrollbar->connect("ValueChanged", [&selectPkmnMenu](tgui::Scrollbar::Ptr scrollbar){
		for (int i = 0; i < 256; i++) {
			selectPkmnMenu.get("pkmnSelect" + std::to_string(i))->setPosition(10 + (i % 6) * 128, 10 + (i / 6) * 148 - scrollbar->getValue());
			selectPkmnMenu.get("pkmnFrontSelect" + std::to_string(i))->setPosition(11 + (i % 6) * 128, 11 + (i / 6) * 148 - scrollbar->getValue());
			selectPkmnMenu.get("pkmnNameSelect" + std::to_string(i))->setPosition(11 + (i % 6) * 128, 138 + (i / 6) * 148 - scrollbar->getValue());
		}
	}, scrollbar);
	selectPkmnMenu.add(scrollbar, "scrollbar");

	for (int i = 0; i < 256; i++) {
		selectPkmnMenu.add(makeButton("", 10 + (i % 6) * 128, 10 + (i / 6) * 148, [&menu, i, &gui, &selectPkmnMenu, &game, &id](tgui::Button::Ptr){
			game.changePokemon(
				id,
				game.getPokemonTeam()[id].getNickname(),
				game.getPokemonTeam()[id].getLevel(),
				PokemonGen1::pokemonList.at(i),
				std::vector<PokemonGen1::Move>(game.getPokemonTeam()[id].getMoveSet())
			);
			updatePokemonTeam(gui, game);
			menu = 0;
		}, 128, 128), "pkmnSelect" + std::to_string(i));
		selectPkmnMenu.add(makePicture("assets/front_sprites/" + std::to_string(i) + "_front.png", 11 + (i % 6) * 128, 11 + (i / 6) * 148, 126, 126), "pkmnFrontSelect" + std::to_string(i));
		selectPkmnMenu.add(makeTextBox(10 + (i % 6) * 128, 138 + (i / 6) * 148, 128, 20, intToHex(i) +  " " + PokemonGen1::pokemonList[i].name), "pkmnNameSelect" + std::to_string(i));
	}


	//Move select
	tgui::Scrollbar::Ptr scrollbar2 = tgui::Scrollbar::create();
	scrollbar2->setPosition(780, 10);
	scrollbar2->setSize(18, 620);
	scrollbar2->setMaximum(12748);
	scrollbar2->setViewportSize(640);
	scrollbar2->connect("ValueChanged", [&selectMovePanel](tgui::Scrollbar::Ptr scrollbar){
		for (int i = 0; i < 256; i++) {
			selectMovePanel.get("moveSelect" + std::to_string(i))->setPosition(10 + (i % 3) * 256, 10 + (i / 3) * 148 - scrollbar->getValue());
			selectMovePanel.get("moveSelectButton" + std::to_string(i))->setPosition(10 + (i % 3) * 256, 10 + (i / 3) * 148 - scrollbar->getValue());
			selectMovePanel.get("moveSelectCategoryPicture" + std::to_string(i))->setPosition(11 + (i % 3) * 256, 26 + (i / 3) * 148 - scrollbar->getValue());
			selectMovePanel.get("moveSelectTypePicture" + std::to_string(i))->setPosition(217 + (i % 3) * 256, 11 + (i / 3) * 148 - scrollbar->getValue());
		}
	}, scrollbar2);
	selectMovePanel.add(scrollbar2, "scrollbar2");

	for (int i = 0; i < 256; i++) {
		const auto &move = PokemonGen1::availableMoves.at(i);
		std::stringstream stream;

		stream << std::endl;
		switch (move.getCategory()) {
		case PokemonGen1::PHYSICAL:
			stream << "        Physical" << std::endl;
			break;
		case PokemonGen1::SPECIAL:
			stream << "        Special" << std::endl;
			break;
		case PokemonGen1::STATUS:
			stream << "        Status" << std::endl;
			break;
		}
		stream << "Power:      " << (move.getPower() ? std::to_string(move.getPower()) : "--") << std::endl;
		stream << "Accuracy: " << (move.getAccuracy() <= 100 ? std::to_string(move.getAccuracy()) : "--") << std::endl;
		stream << static_cast<int>(move.getPP()) << "/" << static_cast<int>(move.getMaxPP()) << " PP" << std::endl;

		selectMovePanel.add(makeTextBox(10 + (i % 3) * 256, 10 + (i / 3) * 148, 256, 128, stream.str()), "moveSelect" + std::to_string(i));
		selectMovePanel.add(makeButton(move.getName(), 10 + (i % 3) * 256, 10 + (i / 3) * 148, [&menu, i, &gui, &selectMovePanel, &game, &id](tgui::Button::Ptr) {
			std::vector<PokemonGen1::Move> moves = game.getPokemonTeam()[id % 10].getMoveSet();

			moves[id / 10] = PokemonGen1::availableMoves.at(i);

			game.changePokemon(
				id % 10,
				game.getPokemonTeam()[id % 10].getNickname(),
				game.getPokemonTeam()[id % 10].getLevel(),
				PokemonGen1::pokemonList.at(game.getPokemonTeam()[id % 10].getID()),
				moves
			);
			updatePokemonTeam(gui, game);
			menu = 0;
		}, 100, 16), "moveSelectButton" + std::to_string(i));
		selectMovePanel.add(makePicture("assets/types/type_" + toLower(typeToString(move.getType())) + ".png", 217 + (i % 3) * 256, 11 + (i / 3) * 148, 48, 16), "moveSelectTypePicture" + std::to_string(i));
		selectMovePanel.add(makePicture(
			move.getCategory() == PokemonGen1::PHYSICAL ? "assets/physical.png" : (
			move.getCategory() == PokemonGen1::SPECIAL ? "assets/special.png" :
			"assets/status.png")
			, 11 + (i % 3) * 256, 26 + (i / 3) * 148, 32, 14
		), "moveSelectCategoryPicture" + std::to_string(i));
	}
}

void mainMenu(tgui::Gui &gui, sf::RenderWindow &window, PokemonGen1::GameHandle &game, const std::string &trainerName)
{
	unsigned id = 0;
	unsigned menu = 0;
	tgui::Gui panel = tgui::Gui(window);
	tgui::Gui panel2 = tgui::Gui(window);

	makeMainMenuGUI(gui, panel, panel2, game, id, menu);
	window.setTitle(trainerName + " - Main menu");
	while (window.isOpen()) {
		sf::Event event;

		while (window.pollEvent(event)) {
			if (event.type == sf::Event::Closed)
				window.close();
			switch (menu) {
			case 0:
				gui.handleEvent(event);
				break;
			case 1:
				panel.handleEvent(event);
				break;
			default:
				panel2.handleEvent(event);
			}
		}

		gui.get<tgui::Button>("ready")->setText(game.isReady() ? "You are ready for battle" : "You are not ready");
		if (game.isConnected())
			switch (game.getStage()) {
			case PokemonGen1::PKMN_CENTER:
				gui.get<tgui::TextBox>("status")->setText("Opponent not ready");
				break;
			case PokemonGen1::PINGING_OPPONENT:
				gui.get<tgui::TextBox>("status")->setText("Waiting for opponent to save the game");
                                break;
			case PokemonGen1::ROOM_CHOOSE:
				gui.get<tgui::TextBox>("status")->setText("Choosing colosseum");
				break;
			case PokemonGen1::PING_POKEMON_EXCHANGE:
				gui.get<tgui::TextBox>("status")->setText(game.isReady() ? "Waiting for opponent to start the game" : "Waiting for you to be ready");
				break;
			case PokemonGen1::EXCHANGE_POKEMONS:
				gui.get<tgui::TextBox>("status")->setText("Exchanging battle data");
				break;
			case PokemonGen1::BATTLE:
				gui.get<tgui::TextBox>("status")->setText("In battle");
				break;
			}
		else
			gui.get<tgui::TextBox>("status")->setText("Disconnected");
		window.clear();
		switch (menu) {
		case 0:
			gui.draw();
			break;
		case 1:
			panel.draw();
			break;
		default:
			panel2.draw();
		}
		window.display();
	}
}

void gui(const std::string &trainerName)
{
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
		nullptr,
		false,
		getenv("MIN_DEBUG")
	);
	sf::RenderWindow window{{800, 640}, trainerName};
	tgui::Gui gui{window};

	handler.setTeamSize(6);
	window.setFramerateLimit(60);
	handler.setReady(false);
	mainMenu(gui, window, handler, trainerName);
}