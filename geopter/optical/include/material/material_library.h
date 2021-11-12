/*******************************************************************************
** Geopter
** Copyright (C) 2021 Hiiragi
** 
** This file is part of Geopter.
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License as published by the Free Software Foundation; either
** version 2.1 of the License, or (at your option) any later version.
** 
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Lesser General Public License for more details.
** 
** You should have received a copy of the GNU Lesser General Public
** License along with this library; If not, see <http://www.gnu.org/licenses/>.
********************************************************************************
**           Author: Hiiragi                                   
**          Website: https://github.com/heterophyllus/Geopter
**          Contact: heterophyllus.work@gmail.com                          
**             Date: May 16th, 2021                                                                                          
********************************************************************************/

#ifndef MATERIALLIBRARY_H
#define MATERIALLIBRARY_H

#include <vector>
#include <string>
#include <memory>

#include "material/air.h"
#include "material/glass_catalog.h"

namespace geopter{


class MaterialLibrary
{
public:
    MaterialLibrary();
    ~MaterialLibrary();

    void clear();

    /** get glass catalog object pointer */
    GlassCatalog* glass_catalog(const std::string catname) const;
    GlassCatalog* glass_catalog(int i) const;

    /** Return number of loaded glass catalogs */
    int catalog_count() const;

    /** load AGF files  */
    bool load_agf_files(const std::vector<std::string>& agf_paths);

    std::shared_ptr<Material> find(std::string material_name);

    static std::shared_ptr<Air> air();

private:
    std::vector< std::unique_ptr<GlassCatalog> > catalogs_;
    std::vector< std::shared_ptr<Material> > materials_;
    static std::shared_ptr<Air> air_;
};


} // namespace geopter

#endif // MATERIALLIBRARY_H