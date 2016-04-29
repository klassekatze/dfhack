#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "DataDefs.h"

using namespace DFHack;

#include "dwarfgui.pb.h"

#include "RemoteServer.h"

using namespace dwarfgui;

#include "df/world.h"
#include "df/unit.h"
#include "df/item.h"
#include "df/general_ref.h"
#include "df/items_other_id.h"
#include "df/unit_inventory_item.h"
#include "df/caste_body_info.h"
#include "df/body_part_raw.h"
#include "df/body_part_template.h"
#include "df/general_ref_contains_itemst.h"
using namespace df::enums;
#include "modules/Items.h"
#include "df/tool_uses.h"

#include "modules/MapCache.h"
#include "modules/World.h"
#include "modules/Screen.h"
#include "modules/Gui.h"

DFHACK_PLUGIN("dwarfgui");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);

REQUIRE_GLOBAL(ui);
REQUIRE_GLOBAL(world);


//taken from http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>

// trim from start (in place)
static inline void ltrim(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s) {
	ltrim(s);
	rtrim(s);
}
//end copied trim functions.
//there may be something like this already in dfhack, but this was quicker, no research




static command_result GetInventory(color_ostream &stream, const InventoryRequest *invent, InventoryReply *out);
static command_result EquipInventoryItem(color_ostream &stream, const InventoryEquipRequest *req, InventoryEquipReply *out);
static command_result MoveInventoryItem(color_ostream &stream, const InventoryMoveRequest *req, InventoryMoveReply *out);
//static command_result MoveInventory(color_ostream &stream, const InventoryMoveRequest *in, InventoryMoveReply *out);

static command_result dwarfguicmd(color_ostream &out, std::vector <std::string> &parameters)
{
	/*CoreSuspender suspend;

	std::string cmd;
	if (!parameters.empty())
		cmd = parameters[0];

	if (cmd == "squad")
	{
		//df.world.unit
	}*/

	return CR_OK;
}


DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{
	if (world && ui) {
		commands.push_back(PluginCommand(
			"dwarfgui", "Remote access interfaces for dwarfgui", dwarfguicmd, false,
			""
			));
		Core::print("DORFGUI");
		//if (Core::getInstance().isWorldLoaded())
		//	plugin_onstatechange(out, SC_WORLD_LOADED);
	}

	return CR_OK;
}


DFhackCExport RPCService *plugin_rpcconnect(color_ostream &)
{
	RPCService *svc = new RPCService();
	svc->addFunction("GetInventory", GetInventory);
	svc->addFunction("EquipInventoryItem", EquipInventoryItem);
	svc->addFunction("MoveInventoryItem", MoveInventoryItem);
	//EquipInventoryItem(color_ostream &stream, const InventoryEquipRequest *req, InventoryEquipReply *out)
	return svc;
}



static df::item* id2item(int id)
{
	std::vector<df::item*> &items = world->items.other[items_other_id::IN_PLAY];
	for (size_t i = 0; i < items.size(); i++)
	{
		if (items[i]->id == id)return items[i];
	}
	return NULL;
}

static std::string wornitemlocation(df::unit* target, df::item* item, df::unit_inventory_item::T_mode mode, int bodypart)
{

	std::string res = "";
	std::string partname = "unknown body part";
	if(target != NULL && target->body.body_plan != NULL && target->body.body_plan->body_parts.size() > bodypart &&
		target->body.body_plan->body_parts[bodypart]->name_singular.size() > 0 &&
		target->body.body_plan->body_parts[bodypart]->name_singular[0] != NULL
		)
	{
		partname = *target->body.body_plan->body_parts[bodypart]->name_singular[0];
	}
	/*switch(item->getType())
	{
	case item_type::GLOVES:
	case item_type::HELM:
	case item_type::ARMOR:
	case item_type::PANTS:
	case item_type::SHOES:
	case item_type::BACKPACK:
	case item_type::QUIVER:*/
		switch(mode)
		{
		case df::unit_inventory_item::T_mode::Weapon:
			res += "held in ";
			break;
		case df::unit_inventory_item::T_mode::Worn:
			res += "worn on ";
			break;
		}
		/*if(mode == unit_inventory_item::)
		{
			res += "worn on ";
		}*/

		res += partname;

	//}
	return res;
}


static void buildinv_recurse(df::unit* target, InventoryItem* cur, df::item* item, int mode, int body_part_id)
{

	//InventoryItem* itempkt = out->add_item();
	//itempkt->container(item->)
	std::string name = Items::getDescription(item, 0, true).c_str();
	if(mode > -1)
	{
		std::string loc = wornitemlocation(target, item, (df::unit_inventory_item::T_mode)mode, body_part_id);
		if(loc.length() > 0)name += "("+wornitemlocation(target, item, (df::unit_inventory_item::T_mode)mode, body_part_id)+")";
	}
	cur->set_name(name);
	cur->set_id(item->id);
	cur->set_itemtype(item->getType());
	cur->set_mode(mode);
	cur->set_body_part_id(body_part_id);
	//bool food = item->isEdiblePlant())/* || item->isEdibleRaw()) ||*/ item->isEdibleCarnivore());

	bool storage = true;
	//from createitem.cpp
	switch (item->getType())
	{
	case item_type::FLASK:
		cur->set_is_flask(true);
		break;
	case item_type::BARREL:
	case item_type::BUCKET:
		//cur->set_is_barrel(true);
		//break;
	//case item_type::ANIMALTRAP:
	case item_type::BOX:
	case item_type::BIN:
		cur->set_is_box(true);
		break;
	case item_type::BACKPACK:
	case item_type::QUIVER:
		break;
	case item_type::TOOL:
		if (item->hasToolUse(tool_uses::LIQUID_CONTAINER))
			break;
		if (item->hasToolUse(tool_uses::FOOD_STORAGE))
			break;
		if (item->hasToolUse(tool_uses::SMALL_OBJECT_STORAGE))
			break;
		if (item->hasToolUse(tool_uses::TRACK_CART))
			break;
	default:
		storage = false;
		//out.printerr("The selected item cannot be used for item storage!\n");
		//return CR_FAILURE;
	}
	if(item->isEdiblePlant())cur->set_is_food(true);
	switch (item->getType())
	{
	case item_type::MEAT:
	case item_type::FISH:
		//FISH_RAW?
	case item_type::PLANT:
	case item_type::CHEESE:
	case item_type::FOOD:
	case item_type::EGG://maybe egg, todo subtype edibility or some shit

		cur->set_is_food(true);
	default:
		break;
	}


	//if(item->getType() == item_type::FOOD || item->isEdiblePlant())cur->set_is_food(true);
	if(item->isLiquid())cur->set_is_liquid(true);

	//item->get


	std::vector<df::item*> contents;
	Items::getContainedItems(item, &contents);
	bool is_container = storage;//contents.size() > 0;

	for (auto it = contents.begin(); it != contents.end(); ++it)
	{
		//if (it->)
		{
			//df::general_ref_contains_itemst* ref = virtual_cast<df::general_ref_contains_itemst>(*it);
			InventoryItem* itempkt = cur->add_item();
			df::item* inv_item = *it;//id2item(ref->item_id);
			if(inv_item != NULL)
			{
				is_container = true;
				buildinv_recurse(target, itempkt, inv_item,-1,-1);
			}
		}
	}

	/*std::vector<df::general_ref*> &refs = item->general_refs;
	bool is_container = false;
	for (auto it = refs.begin(); it != refs.end(); ++it)
	{
		if (virtual_cast<df::general_ref_contains_itemst>(*it))
		{
			df::general_ref_contains_itemst* ref = virtual_cast<df::general_ref_contains_itemst>(*it);
			InventoryItem* itempkt = cur->add_item();
			df::item* inv_item = id2item(ref->item_id);
			if(inv_item != NULL)
			{
				is_container = true;
				buildinv_recurse(itempkt, inv_item);
			}
		}
	}*/
	cur->set_container(is_container);
}
//static bool moveToInventory(MapExtras::MapCache &mc, df::item *item, df::unit *unit, df::body_part_raw * targetBodyPart, bool ignoreRestrictions, int multiEquipLimit, bool verbose);
static void buildinv_root(InventoryReply* pkt, df::unit* target)
{
	pkt->set_available(true);
	std::vector<df::unit_inventory_item* > &inv = target->inventory;
	if(target != NULL)
	{
		for (auto it = inv.begin(); it != inv.end(); ++it)
		{
			if((*it)->item != NULL)
			{
				//(*it)->body_part_id
				df::item* item = (*it)->item;
				InventoryItem* itempkt = pkt->add_item();
				buildinv_recurse(target, itempkt, item, (int)(*it)->mode, (int)(*it)->body_part_id);
			}
		}
		//DFHack.print("");
	}
}


static int INVENTORY_ROOT = -1;

static command_result GetInventory(color_ostream &stream, const InventoryRequest *invent, InventoryReply *out)
{
	stream.print("GetInventory called\n");
	if (!Core::getInstance().isWorldLoaded()) {
		//out->set_available(false);
		return CR_OK;
	}
	df::unit* target = world->units.active[0];
	if(invent->has_unit_id())
	{
		int id = invent->unit_id();
		for(int i =0; i < world->units.all.size(); i++)
		{
			df::unit* cur = world->units.all[i];
			if(cur != NULL && cur->id == id)
			{
				target = cur;
				break;
			}
		}
	}

	if(target != NULL)
	{
		buildinv_root(out, target);
	}else
	{
		stream.print("target was null!\n");
	}
	stream.print("ok\n");
	return CR_OK;
}

bool is_in_inventory_item(int item_id, df::item* item)
{
	if(item->id == item_id)
	{
		return true;
	}
	//Units::getVisibleName()->language
	std::vector<df::item*> contents;
	Items::getContainedItems(item, &contents);
	for (auto it = contents.begin(); it != contents.end(); ++it)
	{
		df::item* inv_item = *it;//id2item(ref->item_id);
		if(inv_item != NULL)
		{
			bool res = is_in_inventory_item(item_id,inv_item);
			if(res)return true;
		}
	}
	return false;
}

bool is_in_inventory(int item_id, df::unit* target, bool recurse)
{
	if(target != NULL)
	{
		std::vector<df::unit_inventory_item* > &inv = target->inventory;
		
		for (auto it = inv.begin(); it != inv.end(); ++it)
		{
			if((*it)->item != NULL)
			{
				df::item* item = (*it)->item;
				if((int)item->id == item_id)return true;
				if(recurse)
				{
					bool res = is_in_inventory_item(item_id, item);
					if(res)return true;
				}
			}
		}
	}
	return false;
}


static bool MoveInventoryItem(color_ostream &stream, df::unit* target, int item_id, int destination_id)
{
	//if(destination_id == INVENTORY_ROOT)
	//bool in_inv_root = is_in_inventory(item_id, target,false);
	bool in_inv = is_in_inventory(item_id, target,true);
	bool destination_in_inv = is_in_inventory(destination_id, target,true) || destination_id == INVENTORY_ROOT;
	if(in_inv && destination_in_inv)
	{
		stream.print("in_inv && destination_in_inv");
		df::item* item = id2item(item_id);
		df::item* item_dest = id2item(destination_id);
		if(Items::getContainer(item) == item_dest)return true;
		if(item != NULL && (item_dest != NULL || destination_id == INVENTORY_ROOT))
		{
			stream.print("item != NULL && (item_dest != NULL || destination_id == INVENTORY_ROOT)");
			//erase from current location
			MapExtras::MapCache mc;
			if(Items::remove(mc, item, true))
			{
				stream.print("Items::remove(mc, item, true)");
				if(destination_id == INVENTORY_ROOT)
				{
					return true;
					//yhis should mean we are about to pipe the item into EquipItem(df::item* item, df::unit* unit). it should set it up right.
					//df::unit* target = world->units.active[0];
					//Items::moveToInventory(mc, item, target, df::unit_inventory_item::T_mode::InMouth);

					/*for (auto it = plugin->commands.begin(); it != plugin->commands.end(); ++it)
					{
						if(it->name == "forceequip")
						{

						}
					}*/
					//PluginManager::InvokeCommand(out, const std::string & command, std::vector <std::string> & parameters)
				}else
				{
					Core::print("invoking movetocontainer");
					stream.print("invoking movetocontainer");
					return Items::moveToContainer(mc, item, item_dest);
				}
			}else
			{
				Core::print("failed remove()");
			}

		}else
		{
			Core::print("item is null or dest is null and not root");
		}
		/*if(in_inv_root)
		{
			/*std::vector<df::unit_inventory_item* > &inv = target->inventory;
			for(int i=0; i <inv.size(); i++)
			{
				if(inv[i]->item->id == item_id)
				{

				}
			}
		}else
		{

		}*/
		
	}else
	{
		Core::print("OH NO both arent in inv wat");
	}
	return false;
}

static bool EquipItem_smart(df::item* item, df::unit* unit, bool verbose)
{
	//bool verbose = false;
	df::body_part_raw * targetBodyPart = NULL;
	bool ignoreRestrictions = false;
	MapExtras::MapCache mc;
	int multiEquipLimit = 5;

	//edited forceequip copypasta
	const int const_GloveRightHandedness = 1;
	const int const_GloveLeftHandedness = 2;

	//std::vector<df::body_part_raw*> viables;
	std::map<int,int> partsanditemcount;

	for(int bpIndex = 0; bpIndex < unit->body.body_plan->body_parts.size(); bpIndex++)
	{
		df::body_part_raw * currPart = unit->body.body_plan->body_parts[bpIndex];

		if(currPart == NULL)continue;

		// Inspect the current bodypart
		if (item->getType() == df::enums::item_type::GLOVES && currPart->flags.is_set(df::body_part_raw_flags::GRASP) &&
			((item->getGloveHandedness() == const_GloveLeftHandedness && currPart->flags.is_set(df::body_part_raw_flags::LEFT)) ||
			(item->getGloveHandedness() == const_GloveRightHandedness && currPart->flags.is_set(df::body_part_raw_flags::RIGHT))))
		{
			if (verbose) { Core::print("Hand found (%s)...", currPart->token.c_str()); }
		}
		else if (item->getType() == df::enums::item_type::HELM && currPart->flags.is_set(df::body_part_raw_flags::HEAD))
		{
			if (verbose) { Core::print("Head found (%s)...", currPart->token.c_str()); }
		}
		else if (item->getType() == df::enums::item_type::ARMOR && currPart->flags.is_set(df::body_part_raw_flags::UPPERBODY))
		{
			if (verbose) { Core::print("Upper body found (%s)...", currPart->token.c_str()); }
		}
		else if (item->getType() == df::enums::item_type::PANTS && currPart->flags.is_set(df::body_part_raw_flags::LOWERBODY))
		{
			if (verbose) { Core::print("Lower body found (%s)...", currPart->token.c_str()); }
		}
		else if (item->getType() == df::enums::item_type::SHOES && currPart->flags.is_set(df::body_part_raw_flags::STANCE))
		{
			if (verbose) { Core::print("Foot found (%s)...", currPart->token.c_str()); }
		}
		else if (
			(
			item->getType() == df::enums::item_type::WEAPON ||
			item->getType() == df::enums::item_type::SHIELD ||
			item->getType() == df::enums::item_type::CRUTCH
			)
			&& currPart->flags.is_set(df::body_part_raw_flags::GRASP))
		{
			if (verbose) { Core::print("Weaponslot found (%s)...", currPart->token.c_str()); }
		}

		else
		{
			if (verbose) { Core::print("Skipping ineligible bodypart.\n"); }
			// This body part is not eligible for the equipment in question; skip it
			continue;
		}
		//we didn't continue. this is a part that can nominally accept our item.
		partsanditemcount[bpIndex] = 0;
	}
	//much less a blatant copy paste after this point
	//code changed because I don't want to provide specific part targeting in my GUI -it should infer.
	//If you have a boot on the left foot, it should prefer the right. Same for 2 on left 1 on right.
	//if you have six arms, six swords should go one to each arm and so on.

	for (auto it = unit->inventory.begin(); it != unit->inventory.end(); ++it)
	{
		df::unit_inventory_item * currInvItem = *it;

		if (partsanditemcount.find(currInvItem->body_part_id) != partsanditemcount.end() )
		{
			partsanditemcount[currInvItem->body_part_id] += 1;
		}
	}
	std::vector<std::pair<int,int>> final;//flip it so items : part not part : items
	for (auto it = partsanditemcount.begin(); it != partsanditemcount.end(); ++it)
	{
		final.push_back(std::pair<int,int>(it->second,it->first));
	}
	std::sort(final.begin(), final.end());//ascending order - i.e. lowest item count to top.

	if(final.size() > 0)
	{
		int items_on_part = final[0].first;
		int part_id = final[0].second;

		df::unit_inventory_item::T_mode mode = df::unit_inventory_item::Worn;
		switch(item->getType())
		{
		case item_type::WEAPON:
		case item_type::SHIELD:
		case item_type::CRUTCH:
			mode = df::unit_inventory_item::Weapon;
			//multiEquipLimit = 1;//nevermind. this will be set off by gauntlets. If you use this for evil you are becursed! clearly
			//todo make it count different if the item is a weapon from the start maybe? bah
			break;
		default:
			break;
		}
		if(items_on_part < multiEquipLimit)
		{	
			if (Items::moveToInventory(mc, item, unit, mode, part_id))
			{
				return true;
			}
		}
	}
	return false;
}




static command_result EquipInventoryItem(color_ostream &stream, const InventoryEquipRequest *req, InventoryEquipReply *out)
{
	out->set_success(false);
	df::unit* target = world->units.active[0];
	if(req->has_unit_id())
	{
		int id = req->unit_id();
		for(int i =0; i < world->units.active.size(); i++)
		{
			df::unit* cur = world->units.active[i];
			if(cur != NULL && cur->id == id)
			{
				target = cur;
				break;
			}
		}
	}


	if(target != NULL)
	{
		stream.print("target not null");
		bool in_inv_root = is_in_inventory(req->item_id(), target,false);
		bool in_inv = is_in_inventory(req->item_id(), target,true);
		if(in_inv)
		{
			stream.print("in inv");
			df::item* item = id2item(req->item_id());
			df::item* container = Items::getContainer(item);
			MapExtras::MapCache mc;
			//df::item* item = id2item(req->item_id());
			if(!in_inv_root)
			{
				stream.print("not in inv root");
				
				bool moved = MoveInventoryItem(stream, target, (int)req->item_id(), INVENTORY_ROOT);
				if(!moved)
				{
					stream.print("move failed");
					return CR_OK;
				}
				
			}

			bool success = EquipItem_smart(item, target,true);
			if(success)out->set_success(true);
			else
			{
				stream.print("FDFDHGFDH");
				//SHUT. DOWN.
					
				//EVERYTHING.
				if(in_inv_root)//was originally in our root inventory. SOMEHOW.
				{
					stream.print("wut");
					MapExtras::MapCache mc;
					Items::moveToInventory(mc, item, target, df::unit_inventory_item::T_mode::Strapped);
				}else
				{
					stream.print("rebag");
					//in a bag or some shit.
					Items::moveToContainer(mc, item, container);
				}
				//hopefully everything isn't ruined..
			}
		}
	}
	return CR_OK;
}


static command_result MoveInventoryItem(color_ostream &stream, const InventoryMoveRequest *req, InventoryMoveReply *out)
{
	Core::print("MoveInventoryItem()");
	//out->set_success(false);
	df::unit* target = world->units.active[0];
	if(req->has_unit_id())
	{
		int id = req->unit_id();
		for(int i =0; i < world->units.active.size(); i++)
		{
			df::unit* cur = world->units.active[i];
			if(cur != NULL && cur->id == id)
			{
				target = cur;
				break;
			}
		}
	}


	if(req->destination_id() == INVENTORY_ROOT)
	{
		out->set_error("Can't move shit to inventory root, equip instead");
		out->set_success(false);
		return CR_OK;//nope, only via equip
	}
	bool in_inv_root = is_in_inventory(req->item_id(), target,false);
	bool in_inv = is_in_inventory(req->item_id(), target,true);
	if(!in_inv)out->set_error("WHAT HAVE YOU DONE! id is "+req->item_id());

	stream.print("sup");

	bool res = MoveInventoryItem(stream, target, req->item_id(), req->destination_id());
	out->set_success(res);


	/*bool in_inv = is_in_inventory(req->item_id(), target);
	bool in_inv_dest = is_in_inventory(req->destination_id(), target);
	if(in_inv && in_inv_dest)
	{

	}*/
	return CR_OK;
}




enum T_AdventureItemMenuIteratorState : int {
	OFF = 0,
	OPEN_MENU = 1,
	SCROLLING,
};
T_AdventureItemMenuIteratorState AdventureItemMenuIteratorState = OFF;
std::string AdventureItemMenuTarget = "";

/*
static std::map<std::string,int> create_map()
{
	std::map<std::string,int> m = std::map<std::string,int> ();
	/*OPTION1...
	OPTION20*-/
	for(int i = 0; i < 20; i++)
	{
		std::string s;
		char label = 'a'+i;
		s.push_back(label);
		int key = interface_key::OPTION1+i;
		m[s] = key;
	}
	return std::map<std::string,int> ();
}
std::map<std::string,int> keymap = create_map();*/
//SECONDSCROLL_PAGEDOWN to page along


static std::vector<std::string> getscreen()
{
	df::coord2d scrsize = Screen::getWindowSize();
	std::vector<std::string> v;
	for(int y=0; y < scrsize.y; y++)
	{
		std::string line;
		for(int x=0; x < scrsize.x; x++)
		{
			char c = Screen::readTile(x,y).ch;
			if(c == 0)c = ' ';
			line.push_back(c);
		}
		trim(line);
		v.push_back(line);
	}
	return v;
}
static bool compscreens(std::vector<std::string> a, std::vector<std::string> b, bool skiptop)//skiptop to avoid the top line w/flickery FPS overlay false flagging us
{
	bool ident = true;
	int size = a.size();
	if(size > b.size())size = b.size();
	for(int i = (skiptop ? 1 : 0); i < size; i++)
	{
		if(a[i] != b[i])
		{
			ident = false;
			break;
		}
	}
	return ident;
}
static int searchscreen(std::vector<std::string> a, std::string query)
{
	for(int i = 0; i < a.size(); i++)
	{
		if(a[i].find(query) != -1)
		{
			return i;
		}
	}
	return -1;
}

//todo queue up these so you can dropdropdropdrop. after showing it works once, anyway.
//might just do drops hardcode, leave this for eating and other strange things. hopefully that won't fuck up somehow leaving me 10000 pounds.
static void setupMenuIterator(df::interface_key objective, std::string item)
{
	if(AdventureItemMenuIteratorState == OFF)
	{
		std::set<df::interface_key> keys;
		keys.insert(objective);
		Gui::getCurViewscreen(true)->feed(&keys);
		AdventureItemMenuIteratorState = SCROLLING;
	}
}

static std::vector<std::string> last_screen;
static int abort_counter = 0;
DFhackCExport DFHack::command_result plugin_onupdate (color_ostream &out)//(DFHack::Core* pCore)
{
	{
		static int counter = 0;
		if (++counter < 5)
			return DFHack::CR_OK;
		counter = 0;
		if(AdventureItemMenuIteratorState == OFF)return DFHack::CR_OK;

		t_gamemodes gm;
		World::ReadGameMode(gm);

		if(gm.g_mode == game_mode::ADVENTURE)
		{
			if(AdventureItemMenuIteratorState == SCROLLING)
			{
				++abort_counter;
				if(abort_counter > 100)
				{
					AdventureItemMenuIteratorState = OFF;
					abort_counter = 0;
					//cancel failboating
				}
				std::vector<std::string> screen = getscreen();
				int line = searchscreen(screen, AdventureItemMenuTarget);
				if(line != -1 && screen[line].find(AdventureItemMenuTarget) != -1)
				{
					df::interface_key button = static_cast<df::interface_key>( (interface_key::OPTION1 + line) - 2 );//2 rows down to 'a'
					std::set<df::interface_key> keys;
					keys.insert(button);
					Gui::getCurViewscreen(true)->feed(&keys);

					last_screen.clear();
					AdventureItemMenuIteratorState = OFF;
				}else
				{
					if(screen != last_screen)
					{
						//screen = get
						std::set<df::interface_key> keys;
						keys.insert(interface_key::SECONDSCROLL_PAGEDOWN);
						Gui::getCurViewscreen(true)->feed(&keys);
						//////
						last_screen = screen;
					}//else waiting on df to catch up
				}
				//AdventureItemMenuTarget
			}
		}
	}

	return DFHack::CR_OK;
}
