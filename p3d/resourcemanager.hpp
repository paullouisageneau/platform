/***************************************************************************
 *   Copyright (C) 2006-2016 by Paul-Louis Ageneau                         *
 *   paul-louis (at) ageneau (dot) org                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.           *
 ***************************************************************************/

#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include "p3d/include.hpp"
#include "p3d/resource.hpp"
#include "pla/string.hpp"

namespace pla
{

class ResourceManager
{
public :
	// Recupere une ressource
	template <class T> sptr<T> get(const String& name) const;

	// Ajoute une ressource
	void add(const String& name, sptr<Resource> resource);

	// Retire une ressource
	void remove(const String& name);

	// Retire les ressources
	void flush(void);

private :
	//Table contenant les ressources associees a leur nom de fichier
	std::map<String, sptr<Resource> > mResources;
};

// Renvoie un pointeur sur une ressource déjà chargée (NULL si non trouvée)
template <class T>
inline sptr<T> ResourceManager::get(const String &name) const
{
    // Recherche de la ressource
    auto it = mResources.find(name);

    // Si on l'a trouvée on la renvoie, sinon on renvoie NULL
    if (it != mResources.end()) return std::dynamic_pointer_cast<T>(it->second);
    else return NULL;
}

}

#endif // RESOURCEMANAGER_H

