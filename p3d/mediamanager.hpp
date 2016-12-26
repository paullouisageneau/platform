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

#ifndef P3D_MEDIAMANAGER_H
#define P3D_MEDIAMANAGER_H

#include "p3d/include.hpp"
#include "p3d/loader.hpp"
#include "p3d/resourcemanager.hpp"

namespace pla
{

template <class T> struct MediaHandler
{
    std::map<String, sptr<Loader<T> > > loaders;
};

class Shader;
class Program;

class MediaManager :
	public MediaHandler<Shader>,
	public MediaHandler<Program>
{

public:
	MediaManager(sptr<ResourceManager> resourceManager);
        ~MediaManager(void);

	// Ajoute un repertoire de recherche de medias
	void addPath(String path);

	 // Enregistre un nouveau chargeur
	template <class T> void registerLoader(Loader<T>* loader, const String &extensions);

	// Supprime un chargeur
	template <class T> void unregisterLoader(const String &extension);

 	// Charge a partir d'un fichier
	template <class T> sptr<T> load(const String &filename) const;

	 // Retourne la ressource si deja chargee, sinon charge
	template <class T> sptr<T> get(const String &filename) const;

	// Trouve un fichier
	String findMedia(String filename) const;

private :
	// Trouve un loader
	template <class T> Loader<T> &findLoader(const String &filename) const;

	sptr<ResourceManager> mResourceManager;
	std::set<String> mPaths; // Liste des chemins de recherche
};

// Enregistre un nouveau chargeur de media
template <class T>
void MediaManager::registerLoader(Loader<T>* loader, const String &extensions)
{
	if(!loader) return;

	// Récupération des extensions
    	std::list<String> lst;
    	extensions.explode(lst,',');
	while(!lst.empty())
	{
		String ext = lst.front().toLower();
		ext.trim();
		lst.pop_front();
		MediaHandler<T>::loaders.insert(make_pair(ext,loader));
	}
}

// Supprime un chargeur
template <class T>
void MediaManager::unregisterLoader(const String &extensions)
{
	MediaHandler<T>::loaders.erase(extensions);
}


// Charge un media
template <class T>
sptr<T> MediaManager::load(const String &filename) const
{
	// Recherche du fichier dans les répertoires enregistrés
	String fullpath = findMedia(filename);

	// On appelle le loader approprié
	sptr<T> media = findLoader<T>(filename).load(fullpath);

	// On enregistre la ressource
	mResourceManager->add(fullpath,media);

	return media;
}

 // Retourne la ressource si déjà chargée, sinon charge
template <class T>
sptr<T> MediaManager::get(const String &filename) const
{
	// Recherche directement la ressource
	sptr<T> media = mResourceManager->get<T>(filename);
	if(media) return media;

	// Recherche du fichier dans les répertoires enregistrés
	String fullpath = findMedia(filename);

	// Recherche la ressource
	media = mResourceManager->get<T>(fullpath);
	if(media) return media;

	// Sinon on appelle le loader approprié
	media = findLoader<T>(filename).load(fullpath);

	// On enregistre la ressource
	mResourceManager->add(fullpath,media);

	return media;
}

// Cherche le loader correspondant à un fichier donné
template <class T>
Loader<T> &MediaManager::findLoader(const String &filename) const
{
	int p = filename.lastIndexOf('.');
	if(p != String::NotFound)
	{
		String extension(filename,++p);

		// Recherche de l'extension dans la map de loaders
		auto it = MediaHandler<T>::loaders.find(extension.toLower());

		// On renvoie le loader approprié
		if(it != MediaHandler<T>::loaders.end())
			return *it->second;
	}

	throw LoadingFailed(filename, "Aucun loader ne prend en charge ce format de fichier");
}

}

#endif
