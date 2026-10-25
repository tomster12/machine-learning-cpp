#pragma once

#include <array>
#include <optional>
#include "CommonImpl.h"
#include "DogfightArena.h"
#include "DogfightPilot.h"

enum class DogfightOutcome { FIGHTING, WON, LOST, SURVIVED };

class DogfightAgent : public tbml::ga::Agent<NNGenome>
{
public:
	static constexpr size_t MAX_STEPS = 1800;
	static constexpr size_t DECISION_INTERVAL = 2;

	DogfightAgent(DogfightAgent::GenomeCPtr&& genome, std::shared_ptr<const DogfightLevel> level, DogfightAgent::GenomeCPtr rival, int rivalGeneration);

	bool evaluate() override;
	void appendPackVertices(sf::VertexArray& vertices) const;
	void renderElite(sf::RenderWindow* window) const;

	std::vector<DogfightArena::Event> takeEvents() { return arena.takeEvents(); }
	DogfightOutcome getOutcome() const { return outcome; }
	const DogfightArena& getArena() const { return arena; }
	const DogfightLevel& getLevel() const { return arena.getLevel(); }
	size_t getStep() const { return step; }
	int getRivalGeneration() const { return rivalGeneration; }
	bool hasRival() const { return rivalPilot.has_value(); }
	const tbml::Tensor& getNetInput() const { return pilot.getNetInput(); }
	const tbml::nn::NeuralNetwork& getBrain() const { return pilot.getBrain(); }

private:
	void calculateFitness();
	sf::Color stateColour(std::uint8_t alpha) const;

	DogfightPilot pilot;
	std::optional<DogfightPilot> rivalPilot;
	DogfightArena arena;
	std::array<DogfightShip::Controls, 2> controls;
	DogfightOutcome outcome = DogfightOutcome::FIGHTING;
	int rivalGeneration = 0;
	size_t step = 0;
	size_t aliveSteps = 0;
};
