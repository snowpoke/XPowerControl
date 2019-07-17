// Adds a serializer for std::optional<T> to boost::serialization.
// By default, boost::serialization only has a serializer for boost::optional, so we
// transform from std::optional into boost::optional to serialize.

#include <optional>
#include "boost/serialization/split_free.hpp"
#include "boost/optional.hpp"


namespace boost {
	namespace serialization {
		template<class Archive, class T>
		void load(Archive& ar, std::optional<T>& std_opt, const unsigned int version) {

			// load boost::optional, then transform into std::optional
			boost::optional<T> boost_opt;
			ar >> boost_opt;
			std_opt = (boost_opt) ? std::optional<T>(*boost_opt) : std::optional<T>();
		}

		template<class Archive, class T>
		void save(Archive& ar, const std::optional<T>& std_opt, const unsigned int version) {

			// transform std::optional into boost::optional, then save
			boost::optional<T> boost_opt;
			boost_opt = (std_opt) ? *std_opt : boost::optional<T>();
			ar << boost_opt;
		}

		template<class Archive, class T>
		inline void serialize(Archive& ar, std::optional<T>& std_opt, const unsigned int version) {
			split_free(ar, std_opt, version);
		}
	}
}