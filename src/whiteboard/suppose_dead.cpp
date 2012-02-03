/* $Id$ */
/*
 Copyright (C) 2011 - 2012 by Tommy Schmitz
 Part of the Battle for Wesnoth Project http://www.wesnoth.org

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY.

 See the COPYING file for more details.
 */

/**
 * @file
 */

#include "suppose_dead.hpp"

#include "visitor.hpp"
#include "manager.hpp"
#include "side_actions.hpp"
#include "utility.hpp"

#include "arrow.hpp"
#include "config.hpp"
#include "foreach.hpp"
#include "game_display.hpp"
#include "game_end_exceptions.hpp"
#include "mouse_events.hpp"
#include "play_controller.hpp"
#include "replay.hpp"
#include "resources.hpp"
#include "team.hpp"
#include "unit.hpp"
#include "unit_display.hpp"
#include "unit_map.hpp"

namespace wb
{

	std::ostream& operator<<(std::ostream &s, suppose_dead_ptr sup_d)
	{
		assert(sup_d);
		return sup_d->print(s);
	}

	std::ostream& operator<<(std::ostream &s, suppose_dead_const_ptr sup_d)
	{
		assert(sup_d);
		return sup_d->print(s);
	}

	std::ostream& suppose_dead::print(std::ostream &s) const
	{
		s << "Suppose-dead for unit " << get_unit()->name() << " [" << get_unit()->id() << "] "
				<< "at (" << loc_ << ")";
		return s;
	}

	suppose_dead::suppose_dead(size_t team_index, bool hidden, unit& curr_unit, map_location const& loc)
	: action(team_index,hidden)
	, unit_underlying_id_(curr_unit.underlying_id())
	, unit_id_(curr_unit.id())
	, loc_(loc)
	, valid_(true)
	{
		this->init();
	}

	suppose_dead::suppose_dead(config const& cfg, bool hidden)
	: action(cfg,hidden)
	, unit_underlying_id_(0)
	, unit_id_()
	, loc_(cfg.child("loc_")["x"],cfg.child("loc_")["y"])
	, valid_(true)
	{
		// Construct and validate unit_
		unit_map::iterator unit_itor = resources::units->find(cfg["unit_"]);
		if(unit_itor == resources::units->end())
			throw action::ctor_err("suppose_dead: Invalid underlying_id");

		unit_underlying_id_ = unit_itor->underlying_id();
		unit_id_ = unit_itor->id();

		this->init();
	}

	void suppose_dead::init()
	{
		resources::screen->invalidate(loc_);
	}

	suppose_dead::~suppose_dead()
	{
		//invalidate hex so that skull indicator is properly cleared
		if(resources::screen)
			resources::screen->invalidate(loc_);
	}

	unit* suppose_dead::get_unit() const
	{
		unit_map::iterator itor = resources::units->find(unit_underlying_id_);
		if (itor.valid())
			return &*itor;
		else
			return NULL;
	}

	void suppose_dead::accept(visitor& v)
	{
		v.visit(shared_from_this());
	}

	void suppose_dead::execute(bool& success, bool& complete)
		{success = false;   complete = true;}

	void suppose_dead::apply_temp_modifier(unit_map& unit_map)
	{
		// Remove the unit
		unit const* removed_unit = unit_map.extract(loc_);
		DBG_WB << "Suppose dead: Temporarily removing unit " << removed_unit->name() << " [" << removed_unit->id()
				<< "] from (" << loc_ << ")\n";

		// Just check to make sure we removed the unit we expected to remove
		assert(get_unit() == removed_unit);
	}

	void suppose_dead::remove_temp_modifier(unit_map& unit_map)
	{
		// Just check to make sure the hex is empty
		unit_map::iterator unit_it = resources::units->find(loc_);
		assert(unit_it == resources::units->end());

		// Restore the unit
		unit_map.insert(get_unit());
	}

	void suppose_dead::draw_hex(const map_location& hex)
	{
		if(hex == loc_) //add symbol to hex
		{
			//@todo: Possibly use a different layer
			const display::tdrawing_layer layer = display::LAYER_ARROWS;

			int xpos = resources::screen->get_location_x(loc_);
			int ypos = resources::screen->get_location_y(loc_);

			resources::screen->drawing_buffer_add(layer, loc_, xpos, ypos,
					image::get_image("whiteboard/suppose_dead.png", image::SCALED_TO_HEX));
		}
	}

	void suppose_dead::set_valid(bool valid)
	{
		valid_ = valid;
	}

	config suppose_dead::to_config() const
	{
		config final_cfg = action::to_config();

		final_cfg["type"]="suppose_dead";
		final_cfg["unit_"]=static_cast<int>(unit_underlying_id_);
		final_cfg["unit_id_"]=unit_id_;

		config loc_cfg;
		loc_cfg["x"]=loc_.x;
		loc_cfg["y"]=loc_.y;
		final_cfg.add_child("loc_",loc_cfg);

		return final_cfg;
	}

} // end namespace wb
