#pragma once

#include "Directory.h"
#include "Object.h"


/*
With templates, this could probably be abstracted to do generic trees for other things
but I'm not going to bother with that crap right now.

Note that ObjectTree doesn't actually store its own ObjectTypes;
It's assumed that such ObjectTypes were created elsewhere; and that this is all just some really big & weird iterator machine for
making those ObjectTypes inherit from their ancestors in linear-complexity time at parsetime.

One could also probably use this to generate some fancy debug dump for the program's object tree or something. I'unno, I'm a comment, not a software engineer.
*/
class ObjectTree
{
	/*
	AWAKEN MY CHILD, AND EMBRACE THE GLORY THAT IS YOUR BIRTHRIGHT.
	KNOW THAT I AM THE OVERMIND, THE ETERNAL WILL OF THE SWARM,
	AND THAT YOU HAVE BEEN CREATED TO SERVE ME.
	*/
	struct Node
	{
		Node* root = nullptr;
		std::vector<Node*> children;
		ObjectType* me = nullptr;

		Node()
		{

		}
		Node(ObjectType* moi) // I'm at the pizza hut
			:me(moi)
		{

		}
		Node(Node* daddy) // I'm at the taco bell
			:root(daddy)
		{

		}
		Node(ObjectType* moi, Node* daddy) // I'm at that COMBINATION pizza hut and taco bell
			:root(daddy)
			,me(moi)
		{

		}
	};

	std::vector<Node*> basetypes; // The nodes whose root is root

	//Allows for instant lookup of a given ObjectType's location in the node structure 
	//Can't use ObjectType* as a key unfortunately since sometimes we may know of a type before we know its pointer (such as when /A/B/C is defined before /A or /A/B)
	Hashtable<std::string, Node*> dir2node;

	//Tell derived classes of a new base class we found for them
	void propagate_downstream(ObjectType* base, Node* inheriter)
	{
		ObjectType* derived = inheriter->me;
		if (derived) // If inheriter is not a dummy
		{
			derived->typeproperties.insert(base->typeproperties.begin(), base->typeproperties.end());
			derived->typefuncs.insert(base->typefuncs.begin(), base->typefuncs.end());
			// Passively, this does not override (and thereby clobber) the properties already present in our objecttype, as just a behavior of the std::vector::insert() function.
		}
		for (Node* child : inheriter->children)
		{
			propagate_downstream(base, child);
		}
	}

	//Walk a derived class through all of its ancestors, meeting them and taking on their properties along this journey
	void propagate_upstream(ObjectType* derived, Node* ancestor)
	{
		ObjectType* base = ancestor->me;
		if (base)
		{
			derived->typeproperties.insert(base->typeproperties.begin(), base->typeproperties.end());
			derived->typefuncs.insert(base->typefuncs.begin(), base->typefuncs.end());
			// Passively, this does not override (and thereby clobber) the properties already present in our objecttype, as just a behavior of the std::vector::insert() function.
			
			//FIXME: Metatables need to be rejiggered to allow for derived classes to have ones that are deviant from their base class's metatable.
			if (base->mt)
				derived->mt = base->mt;

			//FIXME: Tables need to be rejiggered to use metatables instead of the ad-hoc structure they currently have.
			if (base->is_table_type)
				derived->is_table_type = true;
		}

		if (ancestor->root)
			propagate_upstream(derived, ancestor->root);
	}
public:

	//Has to pass-as-pointer instead of as-ref because otherwise it'd be (apparently) impossible to get the pointer to store for that reference.
	void append(ObjectType* newtype)
	{
		std::string dir = newtype->get_name();
		std::string base = Directory::DotDot(dir);

		if (dir2node.count(dir)) // If a dummy (or previous definition? hard to tell, we don't check) node already exists for us
		{
			Node* ourdummy = dir2node.at(dir);
			ourdummy->me = newtype;
			if (base != "/") // If we're a derived class
			{
				propagate_upstream(newtype, ourdummy->root);
			}
			for (Node* child : ourdummy->children)
			{
				propagate_downstream(newtype, child);
			}
			return;
		}

		if (base == "/") // If it's a proper base class with no inheritence
		{

			Node* basenode = new Node(newtype);
			basetypes.push_back(basenode);
			dir2node[dir] = basenode;
			return;
		}
		//Else, we're a derived class.

		/*
		So there's some different possibilities for what it is we're to be doing to include this derived class.
		
		1. It's possible its base just exists, in which case we can link it up and call that a success

		2. Alternatively, the derived class may reference classes which we haven't seen before,
		in which case it has to form a family line and stick that line onto the tree once it reaches either root,
		or an ancestor that actually exists and has been seen before.

		All this is an English description of what this is supposed to do. Whether it actually does that is anyone's guess.
		*/

		//Option 1. Our base just exists
		if (dir2node.count(base)) //If we find a class we're the exact child of
		{
			Node* parent = dir2node.at(base);
			Node* ournode = new Node(newtype, parent); // Create child, with this type and this parent
			parent->children.push_back(ournode); // Inform the parent of the birth of their new child
			dir2node[dir] = ournode;

			propagate_upstream(newtype, parent);

			// We know that we have no descendants, because otherwise there would've been a little dummy slot for us at the beginning,
			// created by our child when they were found.
			return;
		}

		//Option 2. Our base doesn't exist yet

		//Begin making a line
		Node* father = new Node();
		Node* ournode = new Node(newtype, father);
		father->children.push_back(ournode);
		dir2node[base] = father;
		dir2node[dir] = ournode;

		//Try again
		base = Directory::DotDot(base);
		Node* top_constructed_ancestor = father;
		do
		{
			if (base == "/") //If we're hit root
			{
				basetypes.push_back(top_constructed_ancestor);
				return;
			}

			if (dir2node.count(base)) // If we've found an already-existant node that's an ancestor
			{
				Node* missing_link = dir2node.at(base);
				missing_link->children.push_back(top_constructed_ancestor);
				top_constructed_ancestor->root = missing_link;

				//Now lets make this distant derived class inherit from this deeply-ancestored base class

				propagate_upstream(newtype, missing_link); // It can skip all the way up here since we know that everything in the middle are dummy nodes with no inheritence data available

				return;
			}

			//If this ancestor doesn't exist, construct a dummy for it
			Node* new_ancestor = new Node();
			top_constructed_ancestor->root = new_ancestor;
			dir2node[base] = new_ancestor;
			top_constructed_ancestor = new_ancestor;

			//And search ever onwards!
			base = Directory::DotDot(base);
		} while (true);
	}

	/*
	A function called recursively by dump() to dump each node.
	*/
	void dump_node(Node* nude, int indent)
	{
		std::string ind = "";
		if (indent)
			ind = std::string(indent, ' ');

		if (!nude) // Uh-oh!
		{
			std::cout << ind << "NULLPTR\n";
			return;
		}
		if (!nude->me) // Uh-oh, again!
		{
			std::cout << ind << "NODE\n";
			return;
		}

		std::cout << ind << Directory::lastword(nude->me->object_type.to_string()) << std::endl;
		for (Node* child : nude->children)
		{
			dump_node(child, indent + 1);
		}
	}

	/*
	Dumps into std::cout a big ol' objecttype tree.
	*/
	void dump()
	{
		for (Node* n : basetypes)
		{
			dump_node(n, 0);
		}
	}
};