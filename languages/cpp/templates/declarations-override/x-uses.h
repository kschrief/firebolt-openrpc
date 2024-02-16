        /*
         ${method.name}
         ${method.description}
         */
        void request${method.Name}(const ${method.Name}Usage& type, const std::optional<std::string>& message, I${info.Title}AsyncResponse& response, Firebolt::Error *err = nullptr ) override;
        void abort${method.Name}(I${info.Title}AsyncResponse& response, Firebolt::Error *err = nullptr) override;
