#pragma once

#include <numeric>
#include "Utility.h"
#include "ThreadPool.h"

// Require SFML to be imported
// TODO: Figure out if this is best way
namespace sf
{
	class RenderWindow;
}

namespace tbml
{
	namespace ga
	{
		// TGenome: Self
		template<class TGenome>
		class Genome
		{
		public:
			using GenomeCPtr = std::shared_ptr<const TGenome>;

			Genome(const Genome&) = delete;
			Genome& operator=(const Genome&) = delete;
			Genome(const Genome&&) = delete;
			Genome& operator=(const Genome&&) = delete;
			virtual GenomeCPtr crossover(const GenomeCPtr& otherGenome, float mutateChance = 0.0f) const = 0;

		protected:
			Genome() = default;
			~Genome() = default;
		};

		// TGenome: Genome<TGenome>
		template<class TGenome>
		class Agent
		{
		public:
			using GenomeCPtr = std::shared_ptr<const TGenome>;

			Agent(GenomeCPtr&& genome) : genome(std::move(genome)), isFinished(false), fitness(0.0f) {};
			~Agent() = default;
			Agent(const Agent&) = delete;
			Agent& operator=(const Agent&) = delete;
			Agent(const Agent&&) = delete;
			Agent& operator=(const Agent&&) = delete;
			virtual bool evaluate() = 0;
			virtual void render(sf::RenderWindow* window) = 0;
			const GenomeCPtr& getGenome() const { return this->genome; };
			bool getFinished() const { return this->isFinished; };
			float getFitness() const { return this->fitness; };

		protected:
			const GenomeCPtr genome;
			bool isFinished = false;
			float fitness = 0;
		};

		// Seperate interface without template for Genepool
		class IGenepool
		{
		public:
			virtual void configThreading(bool enableMultithreadedStepEvaluation = false, bool enableMultithreadedFullEvaluation = false, bool syncMultithreadedSteps = false) = 0;
			virtual void resetGenepool(int populationSize, float mutationRate) = 0;
			virtual void render(sf::RenderWindow* window) = 0;
			virtual void initializeGeneration() = 0;
			virtual void evaluateGeneration(bool step = false) = 0;
			virtual void iterateGeneration() = 0;
			virtual int getGenerationNumber() const = 0;
			virtual float getBestFitness() const = 0;
			virtual bool getGenepoolInitialized() const = 0;
			virtual bool getGenerationEvaluated() const = 0;
			virtual bool getShowVisuals() const = 0;
			virtual void setShowVisuals(bool showVisuals) = 0;
		};

		using IGenepoolPtr = std::shared_ptr<IGenepool>;

		// TGenome: Genome<TGenome>, TAgent: Agent<TGenome>
		template<class TGenome, class TAgent>
		class Genepool : public IGenepool
		{
		public:
			using GenomeCPtr = std::shared_ptr<const TGenome>;
			using AgentPtr = std::shared_ptr<TAgent>;

			Genepool(std::function<GenomeCPtr(void)> createGenomeFn, std::function<AgentPtr(GenomeCPtr)> createAgentFn)
				: createGenomeFn(createGenomeFn), createAgentFn(createAgentFn)
			{}

			void resetGenepool(int populationSize, float mutationRate)
			{
				// [INITIALIZATION] Initialize new instances
				this->agentPopulation.clear();
				for (int i = 0; i < populationSize; i++)
				{
					GenomeCPtr genome = createGenomeFn();
					AgentPtr agent = createAgentFn(std::move(genome));
					this->agentPopulation.push_back(std::move(agent));
				}

				this->isGenepoolInitialized = true;
				this->populationSize = populationSize;
				this->mutationRate = mutationRate;
				this->currentGeneration = 1;
				this->currentStep = 0;
				this->isGenerationEvaluated = false;

				initializeGeneration();

				std::cout << "Genepool initialized with population size " << this->populationSize << " and mutation rate " << this->mutationRate << "." << std::endl;
			};

			void initializeGeneration() {}

			void evaluateGeneration(bool singleStep)
			{
				if (!this->isGenepoolInitialized) throw std::runtime_error("tbml::GenepoolSimulation: Cannot evaluateGeneration because uninitialized.");
				if (this->isGenerationEvaluated) return;

				// Process generation (multi-threaded)
				if ((this->useThreadedStep && singleStep) || (this->useThreadedFullStep && !singleStep))
				{
					// Helper function to evaluate a subset (captures generation)
					auto evaluateSubset = [&](bool singleStep, int start, int end)
					{
						bool subsetEvaluated = false;
						while (!subsetEvaluated)
						{
							subsetEvaluated = true;
							for (int i = start; i < end; i++) subsetEvaluated &= this->agentPopulation[i]->evaluate();
							if (singleStep) break;
						}
						return subsetEvaluated;
					};

					bool allFinished = true;
					size_t threadCount = static_cast<size_t>(std::min(static_cast<int>(evaluateThreadPool.size()), this->populationSize));
					std::vector<std::future<bool>> threadResults(threadCount);
					int subsetSize = static_cast<int>(ceil((float)this->populationSize / threadCount));

					std::cout << "Evaluating generation " << this->currentGeneration << " using " << threadCount << " threads." << std::endl;

					while (!this->isGenerationEvaluated)
					{
						for (size_t i = 0; i < threadCount; i++)
						{
							int startIndex = (int)i * subsetSize;
							int endIndex = static_cast<int>(std::min(startIndex + subsetSize, this->populationSize));
							threadResults[i] = this->evaluateThreadPool.enqueue([=] { return evaluateSubset(singleStep || syncThreadedFullSteps, startIndex, endIndex); });
						}

						this->isGenerationEvaluated = true;
						for (auto&& result : threadResults) this->isGenerationEvaluated &= result.get();
						this->currentStep++;
						if (singleStep) break;
					}
				}

				// Process generation (single-threaded)
				else
				{
					while (!this->isGenerationEvaluated)
					{
						this->isGenerationEvaluated = true;
						for (auto& inst : this->agentPopulation) this->isGenerationEvaluated &= inst->evaluate();
						this->currentStep++;
						if (singleStep) break;
					}
				}

				if (this->isGenerationEvaluated)
				{
					std::cout << "Generation " << this->currentGeneration << " finished evaluating." << std::endl;
				}
			}

			void iterateGeneration()
			{
				if (!this->isGenepoolInitialized) throw std::runtime_error("tbml::GenepoolSimulation: Cannot iterateGeneration because uninitialized.");
				if (!this->isGenerationEvaluated) return;

				// Sort generation and extract best agent
				std::sort(this->agentPopulation.begin(), this->agentPopulation.end(), [this](const auto& a, const auto& b) { return a->getFitness() > b->getFitness(); });
				const AgentPtr& bestInstance = this->agentPopulation[0];
				this->bestGenome = GenomeCPtr(bestInstance->getGenome());
				this->bestFitness = bestInstance->getFitness();
				std::cout << "Generation " << this->currentGeneration << " iterating, best fitness: " << this->bestFitness << std::endl;

				// Initialize next generation with previous best (Elitism)
				std::vector<AgentPtr> nextGeneration;
				nextGeneration.push_back(std::move(createAgentFn(std::move(this->bestGenome))));

				// [SELECTION] Select all parents to use
				size_t reproduceCount = this->populationSize - 1;
				std::vector<AgentPtr> parentData = selectRoulette(this->agentPopulation, (int)reproduceCount * 2);
				//std::vector<AgentPtr> parentData = selectTournament(this->agentPopulation, reproduceCount * 2, 3);

				for (size_t i = 0; i < reproduceCount; i++)
				{
					// Grab the 2 parents from the selection
					const GenomeCPtr& parentDataA = parentData[i * 2 + 0]->getGenome();
					const GenomeCPtr& parentDataB = parentData[i * 2 + 1]->getGenome();

					// [CROSSOVER], [MUTATION] Crossover and mutate new child genome
					GenomeCPtr childGenome = parentDataA->crossover(parentDataB, this->mutationRate);
					nextGeneration.push_back(createAgentFn(std::move(childGenome)));
				}

				// Set to new generation and update variables
				this->agentPopulation = std::move(nextGeneration);
				this->currentGeneration++;
				this->isGenerationEvaluated = false;
				initializeGeneration();
			};

			void render(sf::RenderWindow* window)
			{
				if (!this->isGenepoolInitialized) throw std::runtime_error("tbml::GenepoolSimulation: Cannot render because uninitialized.");
				if (!this->showVisuals) return;
				for (const auto& inst : agentPopulation) inst->render(window);
			};

			void setShowVisuals(bool showVisuals) { this->showVisuals = showVisuals; }

			void configThreading(bool enableMultithreadedStepEvaluation = false, bool enableMultithreadedFullEvaluation = false, bool syncMultithreadedSteps = false)
			{
				if (enableMultithreadedFullEvaluation && enableMultithreadedStepEvaluation)
					throw std::runtime_error("tbml::GenepoolSimulation: Cannot have both enableMultithreadedFullEvaluation and enableMultithreadedStepEvaluation.");
				if (syncMultithreadedSteps && !enableMultithreadedFullEvaluation)
					throw std::runtime_error("tbml::GenepoolSimulation: Cannot have syncMultithreadedSteps without enableMultithreadedFullEvaluation.");

				this->useThreadedStep = enableMultithreadedStepEvaluation;
				this->useThreadedFullStep = enableMultithreadedFullEvaluation;
				this->syncThreadedFullSteps = syncMultithreadedSteps;

				std::cout << "Genepool threading configured: " <<
					(this->useThreadedStep ? "MultithreadedStepEvaluation " : "") <<
					(this->useThreadedFullStep ? "MultithreadedFullEvaluation " : "") <<
					(this->syncThreadedFullSteps ? "SynchronizedThreadedSteps " : "") << std::endl;
			}

			int getGenerationNumber() const { return this->currentGeneration; }

			GenomeCPtr getBestData() const { return this->bestGenome; }

			float getBestFitness() const { return this->bestFitness; }

			bool getGenepoolInitialized() const { return this->isGenepoolInitialized; }

			bool getGenerationEvaluated() const { return this->isGenerationEvaluated; }

			bool getShowVisuals() const { return this->showVisuals; }

			void setCreateGenomeFn(std::function<GenomeCPtr(void)> createGenomeFn) { this->createGenomeFn = createGenomeFn; }

			void setCreateAgentFn(std::function<AgentPtr(GenomeCPtr)> createAgentFn) { this->createAgentFn = createAgentFn; }

		protected:
			std::function<GenomeCPtr(void)> createGenomeFn;
			std::function<AgentPtr(GenomeCPtr)> createAgentFn;
			bool useThreadedStep = false;
			bool useThreadedFullStep = false;
			bool syncThreadedFullSteps = false;
			bool showVisuals = true;
			int populationSize = 0;
			float mutationRate = 0.0f;

			bool isGenepoolInitialized = false;
			bool isGenerationEvaluated = false;
			int currentGeneration = 0;
			int currentStep = 0;
			GenomeCPtr bestGenome = nullptr;
			float bestFitness = 0.0f;
			ThreadPool evaluateThreadPool;
			std::vector<AgentPtr> agentPopulation;

			static std::vector<AgentPtr> selectRoulette(const std::vector<AgentPtr>& agentPopulation, int selectAmount)
			{
				std::vector<float> fitnessValues;
				for (const auto& agent : agentPopulation) fitnessValues.push_back(agent->getFitness());
				float totalFitness = std::accumulate(fitnessValues.begin(), fitnessValues.end(), 0.0f);

				std::vector<float> fitnessProportions;
				for (const auto& fitness : fitnessValues) fitnessProportions.push_back(fitness / totalFitness);

				std::vector<AgentPtr> selectedAgents;
				for (int i = 0; i < selectAmount; i++)
				{
					float r = fn::getRandomFloat() * totalFitness;
					float cumSum = 0.0f;
					for (size_t j = 0; j < agentPopulation.size(); j++)
					{
						cumSum += fitnessValues[j];
						if (r <= cumSum)
						{
							selectedAgents.push_back(agentPopulation[j]);
							break;
						}
					}
				}

				return selectedAgents;
			}

			static std::vector<AgentPtr> selectTournament(const std::vector<AgentPtr>& agentPopulation, int selectAmount, int tournamentSize)
			{
				std::vector<AgentPtr> selectedAgents;
				for (int i = 0; i < selectAmount; i++)
				{
					std::vector<AgentPtr> tournament;
					for (int j = 0; j < tournamentSize; j++)
					{
						int r = tbml::fn::getRandomInt(0, agentPopulation.size() - 1);
						tournament.push_back(agentPopulation[r]);
					}

					std::sort(tournament.begin(), tournament.end(), [](const auto& a, const auto& b) { return a->getFitness() > b->getFitness(); });
					selectedAgents.push_back(tournament[0]);
				}

				return selectedAgents;
			}
		};
	}
}
