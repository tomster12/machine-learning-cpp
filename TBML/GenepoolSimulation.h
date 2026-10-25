#pragma once

#include <algorithm>
#include <cmath>
#include <numeric>
#include "Utility.h"
#include "ThreadPool.h"

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
			const GenomeCPtr& getGenome() const { return this->genome; };
			bool getFinished() const { return this->isFinished; };
			float getFitness() const { return this->fitness; };

		protected:
			const GenomeCPtr genome;
			bool isFinished = false;
			float fitness = 0;
		};

		struct GenerationStats
		{
			float best = 0.0f;
			float average = 0.0f;
			float mutationRate = 0.0f;
		};

		enum class SelectionType { ROULETTE, TOURNAMENT };

		// Seperate interface without template for Genepool
		class IGenepool
		{
		public:
			virtual ~IGenepool() = default;
			virtual void configThreading(bool enableMultithreadedStepEvaluation = false, bool enableMultithreadedFullEvaluation = false, bool syncMultithreadedSteps = false) = 0;
			virtual void resetGenepool(int populationSize, float mutationRate) = 0;
			virtual void initializeGeneration() = 0;
			virtual void evaluateGeneration(bool singleStep = false) = 0;
			virtual void iterateGeneration() = 0;
			virtual int getGenerationNumber() const = 0;
			virtual float getBestFitness() const = 0;
			virtual size_t getPopulationSize() const = 0;
			virtual float getAgentFitness(size_t index) const = 0;
			virtual const std::vector<GenerationStats>& getHistory() const = 0;
			virtual bool getGenepoolInitialized() const = 0;
			virtual bool getGenerationEvaluated() const = 0;
			virtual float getMutationRate() const = 0;
			virtual void setMutationRate(float rate) = 0;
			virtual void resetMutationRate() = 0;
			virtual bool getAdaptiveMutation() const = 0;
			virtual void setAdaptiveMutation(bool enabled) = 0;
			virtual size_t getEliteCount() const = 0;
			virtual void logInformation() = 0;
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
				this->history.clear();
				for (int i = 0; i < populationSize; i++)
				{
					GenomeCPtr genome = createGenomeFn();
					AgentPtr agent = createAgentFn(std::move(genome));
					this->agentPopulation.push_back(std::move(agent));
				}

				this->isGenepoolInitialized = true;
				this->populationSize = populationSize;
				this->mutationRate = mutationRate;
				this->baseMutationRate = mutationRate;
				this->stagnantGenerations = 0;
				this->currentGeneration = 1;
				this->currentStep = 0;
				this->isGenerationEvaluated = false;

				initializeGeneration();
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
			}

			void iterateGeneration()
			{
				if (!this->isGenepoolInitialized) throw std::runtime_error("tbml::GenepoolSimulation: Cannot iterateGeneration because uninitialized.");
				if (!this->isGenerationEvaluated) return;

				// Sort generation and extract best agent
				std::sort(this->agentPopulation.begin(), this->agentPopulation.end(), [this](const auto& a, const auto& b) { return a->getFitness() > b->getFitness(); });
				this->bestAgent = this->agentPopulation[0];
				this->bestGenome = GenomeCPtr(bestAgent->getGenome());
				this->bestFitness = bestAgent->getFitness();

				// Record generation stats alongside the mutation rate used to breed the next generation
				float totalFitness = 0.0f;
				for (const auto& agent : this->agentPopulation) totalFitness += agent->getFitness();
				if (this->adaptiveMutation) this->adaptMutationRate();
				this->history.push_back({ this->bestFitness, totalFitness / static_cast<float>(this->agentPopulation.size()), this->mutationRate });

				std::cout << "Generation " << this->currentGeneration << " iterating, best fitness: " << this->bestFitness << std::endl;

				// Initialize next generation with the previous best (Elitism), keeping them first and in rank order
				const size_t eliteCount = this->getEliteCount();
				std::vector<AgentPtr> nextGeneration;
				for (size_t i = 0; i < eliteCount; i++) nextGeneration.push_back(createAgentFn(GenomeCPtr(this->agentPopulation[i]->getGenome())));

				// [SELECTION] Select all parents to use
				const size_t reproduceCount = this->populationSize - eliteCount;
				std::vector<AgentPtr> parentData = this->selectParents(static_cast<int>(reproduceCount) * 2);

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

			void configThreading(
				bool enableMultithreadedStepEvaluation = false,
				bool enableMultithreadedFullEvaluation = false,
				bool syncMultithreadedSteps = false)
			{
				if (enableMultithreadedFullEvaluation && enableMultithreadedStepEvaluation)
					throw std::runtime_error("tbml::GenepoolSimulation: Cannot have both enableMultithreadedFullEvaluation and enableMultithreadedStepEvaluation.");
				if (syncMultithreadedSteps && !enableMultithreadedFullEvaluation)
					throw std::runtime_error("tbml::GenepoolSimulation: Cannot have syncMultithreadedSteps without enableMultithreadedFullEvaluation.");

				this->useThreadedStep = enableMultithreadedStepEvaluation;
				this->useThreadedFullStep = enableMultithreadedFullEvaluation;
				this->syncThreadedFullSteps = syncMultithreadedSteps;
			}

			void configSelection(SelectionType type, int tournamentSize = 7)
			{
				this->selectionType = type;
				this->tournamentSize = std::max(2, tournamentSize);
			}

			void configElitism(float fraction) { this->eliteFraction = std::clamp(fraction, 0.0f, 0.5f); }

			float getMutationRate() const { return this->mutationRate; }

			void setMutationRate(float rate) { this->mutationRate = std::clamp(rate, 0.0005f, 1.0f); }

			void resetMutationRate()
			{
				this->mutationRate = this->baseMutationRate;
				this->stagnantGenerations = 0;
			}

			bool getAdaptiveMutation() const { return this->adaptiveMutation; }

			void setAdaptiveMutation(bool enabled)
			{
				this->adaptiveMutation = enabled;
				this->stagnantGenerations = 0;
			}

			size_t getEliteCount() const
			{
				const size_t count = static_cast<size_t>(std::round(static_cast<float>(this->populationSize) * this->eliteFraction));
				return std::clamp<size_t>(count, 1, static_cast<size_t>(std::max(1, this->populationSize)));
			}

			std::vector<AgentPtr>* getAgentPopulation() { return &this->agentPopulation; }

			int getGenerationNumber() const { return this->currentGeneration; }

			AgentPtr getBestAgent() const { return this->bestAgent; }

			GenomeCPtr getBestGenome() const { return this->bestGenome; }

			float getBestFitness() const { return this->bestFitness; }

			size_t getPopulationSize() const { return this->agentPopulation.size(); }

			float getAgentFitness(size_t index) const { return this->agentPopulation[index]->getFitness(); }

			const std::vector<GenerationStats>& getHistory() const { return this->history; }

			bool getGenepoolInitialized() const { return this->isGenepoolInitialized; }

			bool getGenerationEvaluated() const { return this->isGenerationEvaluated; }

			void setCreateGenomeFn(std::function<GenomeCPtr(void)> createGenomeFn) { this->createGenomeFn = createGenomeFn; }

			void setCreateAgentFn(std::function<AgentPtr(GenomeCPtr)> createAgentFn) { this->createAgentFn = createAgentFn; }

			void logInformation()
			{
				if (!this->isGenepoolInitialized) throw std::runtime_error("tbml::GenepoolSimulation: Cannot log because uninitialized.");

				std::cout << "---- Genepool ----" << std::endl;
				std::cout << "Population Size: " << this->populationSize << std::endl;
				std::cout << "Mutation Rate: " << this->mutationRate << std::endl;
				std::cout << "Threading:" << std::endl;
				std::cout << "  Multithreaded Step Evaluation: " << (this->useThreadedStep ? "Enabled" : "Disabled") << std::endl;
				std::cout << "  Multithreaded Full Evaluation: " << (this->useThreadedFullStep ? "Enabled" : "Disabled") << std::endl;
				std::cout << "  Sync Multithreaded Steps: " << (this->syncThreadedFullSteps ? "Enabled" : "Disabled") << std::endl;
				std::cout << std::endl;
			}

		protected:
			std::function<GenomeCPtr(void)> createGenomeFn;
			std::function<AgentPtr(GenomeCPtr)> createAgentFn;
			bool useThreadedStep = false;
			bool useThreadedFullStep = false;
			bool syncThreadedFullSteps = false;
			int populationSize = 0;
			float mutationRate = 0.0f;
			float baseMutationRate = 0.0f;
			bool adaptiveMutation = false;
			int stagnantGenerations = 0;
			float eliteFraction = 0.02f;
			SelectionType selectionType = SelectionType::TOURNAMENT;
			int tournamentSize = 7;

			bool isGenepoolInitialized = false;
			bool isGenerationEvaluated = false;
			int currentGeneration = 0;
			int currentStep = 0;
			AgentPtr bestAgent = nullptr;
			GenomeCPtr bestGenome = nullptr;
			float bestFitness = 0.0f;
			ThreadPool evaluateThreadPool;
			std::vector<AgentPtr> agentPopulation;
			std::vector<GenerationStats> history;

			void adaptMutationRate()
			{
				// Compare against the best of the recent generations: explore harder when stuck, settle down while improving
				constexpr size_t WINDOW = 10;
				constexpr int PATIENCE = 5;
				float recentBest = 0.0f;
				for (size_t i = this->history.size() > WINDOW ? this->history.size() - WINDOW : 0; i < this->history.size(); i++) recentBest = std::max(recentBest, this->history[i].best);

				if (this->history.empty() || this->bestFitness > recentBest * 1.01f)
				{
					this->stagnantGenerations = 0;
					this->mutationRate *= 0.85f;
				}
				else if (++this->stagnantGenerations >= PATIENCE) this->mutationRate *= 1.3f;

				this->mutationRate = std::clamp(this->mutationRate, this->baseMutationRate * 0.25f, this->baseMutationRate * 4.0f);
			}

			std::vector<AgentPtr> selectParents(int selectAmount) const
			{
				if (this->selectionType == SelectionType::ROULETTE) return selectRoulette(this->agentPopulation, selectAmount);
				return selectTournament(this->agentPopulation, selectAmount, this->tournamentSize);
			}

			static std::vector<AgentPtr> selectRoulette(const std::vector<AgentPtr>& agentPopulation, int selectAmount)
			{
				std::vector<float> fitnessValues;
				for (const auto& agent : agentPopulation) fitnessValues.push_back(agent->getFitness());
				float totalFitness = std::accumulate(fitnessValues.begin(), fitnessValues.end(), 0.0f);

				std::vector<AgentPtr> selectedAgents;
				for (int i = 0; i < selectAmount; i++)
				{
					// Fall back to a uniform pick when nobody has any fitness, or rounding runs past the end
					if (totalFitness <= 0.0f)
					{
						selectedAgents.push_back(agentPopulation[fn::getRandomInt(0, static_cast<int>(agentPopulation.size()) - 1)]);
						continue;
					}

					float r = fn::getRandomFloat() * totalFitness;
					float cumSum = 0.0f;
					AgentPtr selected = agentPopulation.back();
					for (size_t j = 0; j < agentPopulation.size(); j++)
					{
						cumSum += fitnessValues[j];
						if (r <= cumSum)
						{
							selected = agentPopulation[j];
							break;
						}
					}
					selectedAgents.push_back(std::move(selected));
				}

				return selectedAgents;
			}

			static std::vector<AgentPtr> selectTournament(const std::vector<AgentPtr>& agentPopulation, int selectAmount, int tournamentSize)
			{
				std::vector<AgentPtr> selectedAgents;
				for (int i = 0; i < selectAmount; i++)
				{
					// Fittest of a few random entrants wins
					const AgentPtr* winner = nullptr;
					for (int j = 0; j < tournamentSize; j++)
					{
						const AgentPtr& entrant = agentPopulation[fn::getRandomInt(0, static_cast<int>(agentPopulation.size()) - 1)];
						if (winner == nullptr || entrant->getFitness() > (*winner)->getFitness()) winner = &entrant;
					}
					selectedAgents.push_back(*winner);
				}

				return selectedAgents;
			}
		};
	}
}
