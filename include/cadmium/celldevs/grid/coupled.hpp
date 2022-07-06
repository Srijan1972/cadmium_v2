/**
 * Abstract base class for grid Cell-DEVS coupled model.
 * Copyright (C) 2022  Román Cárdenas Rodríguez
 * ARSLab - Carleton University
 * GreenLSI - Polytechnic University of Madrid
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef CADMIUM_CELLDEVS_GRID_COUPLED_HPP_
#define CADMIUM_CELLDEVS_GRID_COUPLED_HPP_

#include <nlohmann/json.hpp>
#include <memory>
#include <vector>
#include "cell.hpp"
#include "config.hpp"
#include "scenario.hpp"
#include "utility.hpp"
#include "../core/coupled.hpp"
#include "../../core/exception.hpp"

namespace cadmium::celldevs {
	/**
	 * @brief Abstract implementation of a coupled grid Cell-DEVS model.
	 * @tparam S the type used for representing a cell state.
	 * @tparam V the type used for representing a neighboring cell's vicinities.
	 */
	template <typename S, typename V>
	class GridCellDEVSCoupled: public CellDEVSCoupled<coordinates, S, V> { // TODO meter puntero a función aquí.
		using CellDEVSCoupled<coordinates, S, V>::rawConfig;
		std::shared_ptr<GridScenario> scenario;  //!< Pointer to the scenario configuration
	 public:
		/**
		 *
		 * @param id ID of the coupled grid Cell-DEVS model.
		 * @param configFilePath path to the scenario configuration file
		 */
		GridCellDEVSCoupled(const std::string& id, const std::string& configFilePath): CellDEVSCoupled<coordinates, S, V>(id, configFilePath) {
			nlohmann::json rawScenario = rawConfig.at("scenario");
			auto shape = rawScenario.at("shape").get<coordinates>();
			auto origin = rawScenario.contains("origin")? rawScenario["origin"].get<coordinates>() : coordinates(shape.size(), 0);
			auto wrapped = rawScenario.contains("wrapped") && rawScenario["wrapped"].get<bool>();
			scenario = std::make_shared<GridScenario>(shape, origin, wrapped);
		}

		/**
		 * Adds a new cell to the coupled model. Modelers must implement it to use their custom cell models.
		 * @param cellId ID of the new cell.
		 * @param cellConfig all the configuration parameters related to the cell to be added.
		 */
		virtual void addCell(const coordinates& cellId, const std::shared_ptr<const GridCellConfig<S, V>>& cellConfig) = 0;

		/**
	     * Generates a cell configuration struct from a JSON object.
		 * @param configId ID of the configuration to be loaded.
		 * @param cellConfig cell configuration parameters (JSON object).
		 * @return pointer to the cell configuration created.
		 */
		std::shared_ptr<CellConfig<coordinates, S, V>> loadCellConfig(const std::string& configId, const nlohmann::json& cellConfig) const override {
			return std::make_shared<GridCellConfig<S, V>>(configId, cellConfig, scenario);
		}

		/**
		 * From a cell configuration config, it adds all the required cells to the model.
		 * @param cellConfig target cell configuration struct.
		 */
		void addCells(const std::shared_ptr<CellConfig<coordinates, S, V>>& cellConfig) override {
			auto config = std::dynamic_pointer_cast<GridCellConfig<S, V>>(cellConfig);
			if (config == nullptr) {
				throw CadmiumModelException("Invalid cell configuration data type");
			}
			for (const auto& cellId: config->cellMap) {
				this->addCell(cellId, config);
			}
		}

		/**
		 * After adding all the cells with special configuration, it adds the remaining default cells.
		 * @param defaultConfig default cell configuration struct.
		 */
		void addDefaultCells(const std::shared_ptr<CellConfig<coordinates, S, V>>& defaultConfig) override {
			auto config = std::dynamic_pointer_cast<GridCellConfig<S, V>>(defaultConfig);
			if (config == nullptr) {
				throw CadmiumModelException("Invalid cell configuration data type");
			}
			for (const auto& cellId: *scenario) {
				try {
					auto _cell = Coupled::getComponent(CellDEVSCoupled<coordinates , S, V>::cellId(cellId));
				} catch (const CadmiumModelException& ex) {
					this->addCell(cellId, config);
				}
			}
		}
	};
}

#endif //CADMIUM_CELLDEVS_GRID_COUPLED_HPP_
