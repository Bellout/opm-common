/*
  Copyright 2015 Statoil ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/Section.hpp>
#include <opm/parser/eclipse/EclipseState/Eclipse3DProperties.hpp>
#include <opm/parser/eclipse/EclipseState/SimulationConfig/ThresholdPressure.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/E.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/R.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/T.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/V.hpp>
#include <opm/common/utility/stringhelpers.h>

namespace Opm {

    ThresholdPressure::ThresholdPressure(const Deck& deck,
                                         const Eclipse3DProperties& eclipseProperties)
    {

        if( !Section::hasRUNSPEC( deck ) || !Section::hasSOLUTION( deck ) )
            return;

        RUNSPECSection runspecSection( deck );
        SOLUTIONSection solutionSection( deck );

        bool       thpresOption     = false;
        const bool thpresKeyword    = solutionSection.hasKeyword<ParserKeywords::THPRES>();
        const bool hasEqlnumKeyword = eclipseProperties.hasDeckIntGridProperty( "EQLNUM" );
        int        maxEqlnum        = 0;

        //Is THPRES option set?
        if( runspecSection.hasKeyword<ParserKeywords::EQLOPTS>() ) {
            const auto& eqlopts = runspecSection.getKeyword<ParserKeywords::EQLOPTS>( );
            const auto& rec = eqlopts.getRecord(0);
            for( const auto& item : rec ) {
                if( !item.hasValue( 0 ) ) continue;

                const auto& opt = item.get< std::string >( 0 );
                if( opt == "IRREVERS" )
                    throw std::runtime_error("Cannot use IRREVERS version of THPRES option, not implemented");

                if( opt == "THPRES" )
                    thpresOption = true;
            }
        }

        if( thpresOption && !thpresKeyword ) {
            throw std::runtime_error("Invalid solution section; "
                                     "the EQLOPTS THPRES option is set in RUNSPEC, "
                                     "but no THPRES keyword is found in SOLUTION." );
        }


        //Option is set and keyword is found
        if( thpresOption && thpresKeyword ) {
            if( !hasEqlnumKeyword )
                throw std::runtime_error("Error when internalizing THPRES: EQLNUM keyword not found in deck");

            //Find max of eqlnum
            const auto& eqlnumKeyword = eclipseProperties.getIntGridProperty( "EQLNUM" );
            const auto& eqlnum = eqlnumKeyword.getData();
            maxEqlnum = *std::max_element(eqlnum.begin(), eqlnum.end());

            if (0 == maxEqlnum) {
                throw std::runtime_error("Error in EQLNUM data: all values are 0");
            }


            // Fill threshold pressure table.
            const auto& thpres = solutionSection.getKeyword<ParserKeywords::THPRES>( );

            for( const auto& rec : thpres ) {
                const auto& region1Item = rec.getItem<ParserKeywords::THPRES::REGION1>();
                const auto& region2Item = rec.getItem<ParserKeywords::THPRES::REGION2>();
                const auto& thpressItem = rec.getItem<ParserKeywords::THPRES::VALUE>();

                if( !region1Item.hasValue( 0 ) || !region2Item.hasValue( 0 ) )
                    throw std::runtime_error("Missing region data for use of the THPRES keyword");

                const int r1 = region1Item.get< int >(0);
                const int r2 = region2Item.get< int >(0);
                if (r1 > maxEqlnum || r2 > maxEqlnum) {
                    throw std::runtime_error("Too high region numbers in THPRES keyword");
                }

                if (thpressItem.hasValue(0)) {
                    addBarrier( r1 , r2 , thpressItem.getSIDouble( 0 ) );
                } else
                    addBarrier( r1 , r2 );
            }
        }
    }

    bool ThresholdPressure::hasRegionBarrier(int r1 , int r2) const {
        std::pair<int,int> indexPair = makeIndex(r1,r2);
        if (m_pressureTable.find( indexPair ) == m_pressureTable.end())
            return false;
        else
            return true;
    }


    double ThresholdPressure::getThresholdPressure(int r1 , int r2) const {
        std::pair<int,int> indexPair = makeIndex(r1,r2);
        auto iter = m_pressureTable.find( indexPair );
        if (iter == m_pressureTable.end())
            return 0.0;
        else {
            auto pair_pair = *iter;
            auto value_pair = pair_pair.second;
            bool   valid = value_pair.first;
            double value = value_pair.second;
            if (valid)
                return value;
            else {
                std::string msg = "The THPRES value for regions " + ToString(r1) + " and " + ToString(r2) + " has not been initialized. Using 0.0";
                throw std::invalid_argument(msg);
                return 0;
            }
        }


    }


    std::pair<int,int> ThresholdPressure::makeIndex(int r1 , int r2) {
        if (r1 < r2)
            return std::make_pair(r1,r2);
        else
            return std::make_pair(r2,r1);
    }

    void ThresholdPressure::addPair(int r1 , int r2 , const std::pair<bool , double>& valuePair) {
        std::pair<int,int> indexPair = makeIndex(r1,r2);
        m_pressureTable[indexPair] = valuePair;
    }

    void ThresholdPressure::addBarrier(int r1 , int r2 , double p) {
        std::pair<bool,double> valuePair = std::make_pair(true , p);
        addPair( r1,r2, valuePair );
    }

    void ThresholdPressure::addBarrier(int r1 , int r2) {
        std::pair<bool,double> valuePair = std::make_pair(false , 0);
        addPair( r1,r2, valuePair );
    }

    size_t ThresholdPressure::size() const {
        return m_pressureTable.size();
    }


    bool ThresholdPressure::hasThresholdPressure(int r1 , int r2) const {
        std::pair<int,int> indexPair = makeIndex(r1,r2);
        auto iter = m_pressureTable.find( indexPair );
        if (iter == m_pressureTable.end())
            return false;
        else {
            auto pair_pair = *iter;
            auto value_pair = pair_pair.second;
            return value_pair.first;
        }
    }


} //namespace Opm
